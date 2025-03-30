/*

Copyright (c) 2015, 2017, 2021, Arvid Norberg
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#include "libtorrent/session.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/time.hpp" // for clock_type
#include "libtorrent/aux_/utp_stream.hpp"
#include "libtorrent/session_stats.hpp"

#include "test.hpp"
#include "utils.hpp"
#include "setup_swarm.hpp"
#include "settings.hpp"
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include "simulator/packet.hpp"

using namespace lt;

namespace {

struct pppoe_config final : sim::default_config
{
	int path_mtu(address, address) override
	{
		// this is the size left after IP and UDP headers are deducted
		return 1464;
	}
};

std::int64_t metric(std::vector<std::int64_t> const& counters, char const* key)
{
	auto const idx = lt::find_metric_idx(key);
	return (idx < 0) ? -1 : counters[idx];
}

std::vector<std::int64_t> utp_test(sim::configuration& cfg, int send_buffer_size = 0)
{
	sim::simulation sim{cfg};

	std::vector<std::int64_t> cnt;

	setup_swarm(2, swarm_test::upload | swarm_test::large_torrent | swarm_test::no_auto_stop, sim
		// add session
		, [&](lt::settings_pack& pack) {
		// force uTP connection
			utp_only(pack);
			if (send_buffer_size != 0)
				pack.set_int(settings_pack::send_socket_buffer_size, send_buffer_size);
		}
		// add torrent
		, [](lt::add_torrent_params& params) {
			params.flags |= torrent_flags::seed_mode;
		}
		// on alert
		, [&](lt::alert const* a, lt::session& ses) {
			if (auto ss = alert_cast<session_stats_alert>(a))
				cnt.assign(ss->counters().begin(), ss->counters().end());
		}
		// terminate
		, [&](int const ticks, lt::session& s) -> bool
		{
			if (ticks == 100)
				s.post_session_stats();

			if (ticks > 100)
			{
				if (is_seed(s)) return true;

				TEST_ERROR("timeout");
				return true;
			}
			return false;
		});
	return cnt;
}
}

// TODO: 3 simulate non-congestive packet loss
// TODO: 3 simulate unpredictable latencies
// TODO: 3 simulate proper (taildrop) queues (perhaps even RED and BLUE)

// The counters checked by these tests are proxies for the expected behavior. If
// they change, ensure the utp log and graph plot by parse_utp_log.py look good
// still!

TORRENT_TEST(utp_pmtud)
{
#if TORRENT_UTP_LOG
	lt::aux::set_utp_stream_logging(true);
#endif

	pppoe_config cfg;

	std::vector<std::int64_t> cnt = utp_test(cfg);

	// This is the one MTU probe that's lost. Note that fast-retransmit packets
	// (nor MTU-probes) are treated as congestion. Only packets treated as
	// congestion count as utp_packet_loss.
	TEST_EQUAL(metric(cnt, "utp.utp_fast_retransmit"), 2);
	TEST_EQUAL(metric(cnt, "utp.utp_packet_resend"), 2);

	TEST_EQUAL(metric(cnt, "utp.utp_packet_loss"), 0);

	TEST_EQUAL(metric(cnt, "utp.utp_timeout"), 0);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_in"), 593);
	TEST_EQUAL(metric(cnt, "utp.utp_payload_pkts_in"), 72);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_out"), 604);

	// we don't expect any invalid packets, since we're talking to ourself
	TEST_EQUAL(metric(cnt, "utp.utp_invalid_pkts_in"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_redundant_pkts_in"), 0);
}

TORRENT_TEST(utp_plain)
{
#if TORRENT_UTP_LOG
	lt::aux::set_utp_stream_logging(true);
#endif

	// the available bandwidth is so high the test never bumps up against it
	sim::default_config cfg;

	std::vector<std::int64_t> cnt = utp_test(cfg);

	TEST_EQUAL(metric(cnt, "utp.utp_packet_loss"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_timeout"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_fast_retransmit"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_packet_resend"), 0);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_in"), 590);
	TEST_EQUAL(metric(cnt, "utp.utp_payload_pkts_in"), 77);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_out"), 598);

	// we don't expect any invalid packets, since we're talking to ourself
	TEST_EQUAL(metric(cnt, "utp.utp_invalid_pkts_in"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_redundant_pkts_in"), 0);
}

TORRENT_TEST(utp_buffer_bloat)
{
#if TORRENT_UTP_LOG
	lt::aux::set_utp_stream_logging(true);
#endif

	// 50 kB/s, 500 kB send buffer size. That's 10 seconds
	dsl_config cfg(50, 500000);

	std::vector<std::int64_t> cnt = utp_test(cfg);

	TEST_EQUAL(metric(cnt, "utp.utp_packet_loss"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_timeout"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_fast_retransmit"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_packet_resend"), 0);

	TEST_EQUAL(metric(cnt, "utp.utp_samples_above_target"), 429);
	TEST_EQUAL(metric(cnt, "utp.utp_samples_below_target"), 152);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_in"), 633);
	TEST_EQUAL(metric(cnt, "utp.utp_payload_pkts_in"), 84);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_out"), 633);

	// we don't expect any invalid packets, since we're talking to ourself
	TEST_EQUAL(metric(cnt, "utp.utp_invalid_pkts_in"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_redundant_pkts_in"), 0);
}

// low bandwidth limit, but virtually no buffer
TORRENT_TEST(utp_straw)
{
#if TORRENT_UTP_LOG
	lt::aux::set_utp_stream_logging(true);
#endif

	// 50 kB/s, 500 kB send buffer size. That's 10 seconds
	dsl_config cfg(50, 1500);

	std::vector<std::int64_t> cnt = utp_test(cfg);

	TEST_EQUAL(metric(cnt, "utp.utp_packet_loss"), 64);
	TEST_EQUAL(metric(cnt, "utp.utp_timeout"), 32);
	TEST_EQUAL(metric(cnt, "utp.utp_fast_retransmit"), 67);
	TEST_EQUAL(metric(cnt, "utp.utp_packet_resend"), 130);

	TEST_EQUAL(metric(cnt, "utp.utp_samples_above_target"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_samples_below_target"), 269);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_in"), 394);
	TEST_EQUAL(metric(cnt, "utp.utp_payload_pkts_in"), 53);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_out"), 531);

	// we don't expect any invalid packets, since we're talking to ourself
	TEST_EQUAL(metric(cnt, "utp.utp_invalid_pkts_in"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_redundant_pkts_in"), 0);
}

TORRENT_TEST(utp_small_kernel_send_buf)
{
#if TORRENT_UTP_LOG
	lt::aux::set_utp_stream_logging(true);
#endif

	dsl_config cfg(50000, 1000000, lt::milliseconds(10));

	std::vector<std::int64_t> cnt = utp_test(cfg, 5000);

	TEST_EQUAL(metric(cnt, "utp.utp_packet_loss"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_timeout"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_fast_retransmit"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_packet_resend"), 263);

	TEST_EQUAL(metric(cnt, "utp.utp_samples_above_target"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_samples_below_target"), 1010);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_in"), 1018);
	TEST_EQUAL(metric(cnt, "utp.utp_payload_pkts_in"), 69);

	TEST_EQUAL(metric(cnt, "utp.utp_packets_out"), 1035);

	// we don't expect any invalid packets, since we're talking to ourself
	TEST_EQUAL(metric(cnt, "utp.utp_invalid_pkts_in"), 0);
	TEST_EQUAL(metric(cnt, "utp.utp_redundant_pkts_in"), 0);
}

