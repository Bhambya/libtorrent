/*

Copyright (c) 2009, Andrew Resch
Copyright (c) 2007-2022, Arvid Norberg
Copyright (c) 2015, Steven Siloti
Copyright (c) 2016-2018, 2020-2021, Alden Torres
Copyright (c) 2017, Andrei Kurushin
Copyright (c) 2017, Pavel Pimenov
Copyright (c) 2022, Joris CARRIER
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_DISABLE_EXTENSIONS

#include <functional>
#include <vector>
#include <utility>
#include <numeric>
#include <cstdio>

#include "libtorrent/aux_/bt_peer_connection.hpp"
#include "libtorrent/peer_connection_handle.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/aux_/torrent.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/extensions.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/aux_/random.hpp"
#include "libtorrent/aux_/io_bytes.hpp"
#include "libtorrent/performance_counters.hpp" // for counters
#include "libtorrent/aux_/time.hpp"

#if TORRENT_USE_ASSERTS
#include "libtorrent/hasher.hpp"
#endif

namespace libtorrent {
namespace {

	enum
	{
		// this is the max number of bytes we'll
		// queue up in the send buffer. If we exceed this,
		// we'll wait another second before checking
		// the send buffer size again. So, this may limit
		// the rate at which we can server metadata to
		// 160 kiB/s
		send_buffer_limit = 0x4000 * 10,

		// this is the max number of requests we'll queue
		// up. If we get more requests than this, we'll
		// start rejecting them, claiming we don't have
		// metadata. If the torrent is greater than 16 MiB,
		// we may hit this case (and the client requesting
		// doesn't throttle its requests)
		max_incoming_requests = 1024,
	};

	enum class msg_t : std::uint8_t
	{
		request, piece, dont_have
	};

	int div_round_up(int numerator, int denominator)
	{
		return (numerator + denominator - 1) / denominator;
	}

	struct ut_metadata_peer_plugin;

	struct ut_metadata_plugin final
		: torrent_plugin
	{
		explicit ut_metadata_plugin(aux::torrent& t) : m_torrent(t) {}

		std::shared_ptr<peer_plugin> new_connection(
			peer_connection_handle const& pc) override;

		span<char const> metadata() const
		{
			if (!m_metadata.empty()) return m_metadata;
			if (!m_torrent.valid_metadata()) return {};

			auto const ret = m_torrent.torrent_file().info_section();

#ifdef TORRENT_EXPENSIVE_INVARIANT_CHECKS
			if (m_torrent.torrent_file().info_hashes().has_v1())
			{
				TORRENT_ASSERT(hasher(ret).final()
					== m_torrent.torrent_file().info_hashes().v1);
			}
			if (m_torrent.torrent_file().info_hashes().has_v2())
			{
				TORRENT_ASSERT(hasher256(ret).final()
					== m_torrent.torrent_file().info_hashes().v2);
			}
#endif

			return ret;
		}

		bool received_metadata(ut_metadata_peer_plugin& source
			, span<char const> buf, int piece, int total_size);

		// returns a piece of the metadata that
		// we should request.
		// returns -1 if we should hold off the request
		int metadata_request(bool has_metadata);

		void on_piece_pass(piece_index_t) override
		{
			// if we became a seed, copy the metadata from
			// the torrent before it is deallocated
			if (m_torrent.is_seed())
				metadata();
		}

		void metadata_size(int const size)
		{
			if (m_torrent.valid_metadata()) return;
			if (size <= 0 || size > 4 * 1024 * 1024) return;
			m_metadata.resize(size);
			m_requested_metadata.resize(div_round_up(size, 16 * 1024));
		}

		// explicitly disallow assignment, to silence msvc warning
		ut_metadata_plugin& operator=(ut_metadata_plugin const&) = delete;

	private:
		aux::torrent& m_torrent;

		// this buffer is filled with the info-section of
		// the metadata file while downloading it from
		// peers. Once we have metadata, we seed it directly from the
		// torrent_info of the underlying torrent
		aux::vector<char> m_metadata;

		struct metadata_piece
		{
			metadata_piece() = default;
			int num_requests = 0;
			time_point last_request = min_time();
			std::weak_ptr<ut_metadata_peer_plugin> source;
			bool operator<(metadata_piece const& rhs) const
			{ return num_requests < rhs.num_requests; }
		};

		// this vector keeps track of how many times each metadata
		// block has been requested and who we ended up getting it from
		// std::numeric_limits<int>::max() means we have the piece
		aux::vector<metadata_piece> m_requested_metadata;
	};


	struct ut_metadata_peer_plugin final
		: peer_plugin, std::enable_shared_from_this<ut_metadata_peer_plugin>
	{
		friend struct ut_metadata_plugin;

		ut_metadata_peer_plugin(aux::torrent& t, aux::bt_peer_connection& pc
			, ut_metadata_plugin& tp)
			: m_request_limit(min_time())
			, m_torrent(t)
			, m_pc(pc)
			, m_tp(tp)
		{}

		// can add entries to the extension handshake
		void add_handshake(entry& h) override
		{
			entry& messages = h["m"];
			messages["ut_metadata"] = 2;
			if (m_torrent.valid_metadata())
				h["metadata_size"] = m_tp.metadata().size();
		}

		// called when the extension handshake from the other end is received
		bool on_extension_handshake(bdecode_node const& h) override
		{
			m_message_index = 0;
			if (h.type() != bdecode_node::dict_t) return false;
			bdecode_node const messages = h.dict_find_dict("m");
			if (!messages) return false;

			int index = int(messages.dict_find_int_value("ut_metadata", -1));
			if (index == -1) return false;
			m_message_index = index;

			int metadata_size = int(h.dict_find_int_value("metadata_size"));
			if (metadata_size > 0)
				m_tp.metadata_size(metadata_size);
			else
				m_pc.set_has_metadata(false);

			maybe_send_request();
			return true;
		}

		void write_metadata_packet(msg_t const type, int const piece)
		{
			TORRENT_ASSERT(!m_pc.associated_torrent().expired());

#ifndef TORRENT_DISABLE_LOGGING
			static char const* names[] = {"request", "data", "dont-have"};
			char const* n = "";
			if (type >= msg_t::request && type <= msg_t::dont_have) n = names[static_cast<int>(type)];
			m_pc.peer_log(peer_log_alert::outgoing_message, peer_log_alert::ut_metadata
				, "type: %d (%s) piece: %d", static_cast<int>(type), n, piece);
#endif

			// abort if the peer doesn't support the metadata extension
			if (m_message_index == 0) return;

			entry e;
			e["msg_type"] = static_cast<int>(type);
			e["piece"] = piece;

			char const* metadata = nullptr;
			int metadata_piece_size = 0;

			if (m_torrent.valid_metadata())
				e["total_size"] = m_tp.metadata().size();

			if (type == msg_t::piece)
			{
				TORRENT_ASSERT(piece >= 0 && piece < (m_tp.metadata().size() + 16 * 1024 - 1) / (16 * 1024));
				TORRENT_ASSERT(m_pc.associated_torrent().lock()->valid_metadata());
				TORRENT_ASSERT(m_torrent.valid_metadata());

				int const offset = piece * 16 * 1024;
				metadata = m_tp.metadata().data() + offset;
				metadata_piece_size = std::min(
					int(m_tp.metadata().size()) - offset, 16 * 1024);
				TORRENT_ASSERT(metadata_piece_size > 0);
				TORRENT_ASSERT(offset >= 0);
				TORRENT_ASSERT(offset + metadata_piece_size <= m_tp.metadata().size());
			}

			// TODO: 3 use the aux::write_* functions and the span here instead, it
			// will fit better with send_buffer()
			char msg[200];
			char* header = msg;
			char* p = &msg[6];
			int const len = bencode(p, e);
			int const total_size = 2 + len + metadata_piece_size;
			namespace io = aux;
			io::write_uint32(total_size, header);
			io::write_uint8(aux::bt_peer_connection::msg_extended, header);
			io::write_uint8(m_message_index, header);

			m_pc.send_buffer({msg, len + 6});
			// TODO: we really need to increment the refcounter on the torrent
			// while this buffer is still in the peer's send buffer
			if (metadata_piece_size)
			{
				m_pc.append_const_send_buffer(
					span<char>(const_cast<char*>(metadata), metadata_piece_size), metadata_piece_size);
			}

			m_pc.stats_counters().inc_stats_counter(counters::num_outgoing_extended);
			m_pc.stats_counters().inc_stats_counter(counters::num_outgoing_metadata);
		}

		bool on_extended(int const length
			, int const extended_msg, span<char const> body) override
		{
			if (extended_msg != 2) return false;
			if (m_message_index == 0) return false;

			if (length > 17 * 1024)
			{
#ifndef TORRENT_DISABLE_LOGGING
				m_pc.peer_log(peer_log_alert::incoming_message, peer_log_alert::ut_metadata
					, "packet too big %d", length);
#endif
				m_pc.disconnect(errors::invalid_metadata_message, operation_t::bittorrent, peer_connection_interface::peer_error);
				return true;
			}

			if (!m_pc.packet_finished()) return true;

			error_code ec;
			bdecode_node msg = bdecode(body, ec);
			if (msg.type() != bdecode_node::dict_t)
			{
#ifndef TORRENT_DISABLE_LOGGING
				m_pc.peer_log(peer_log_alert::incoming_message, peer_log_alert::ut_metadata
					, "not a dictionary");
#endif
				m_pc.disconnect(errors::invalid_metadata_message, operation_t::bittorrent, peer_connection_interface::peer_error);
				return true;
			}

			bdecode_node const& type_ent = msg.dict_find_int("msg_type");
			bdecode_node const& piece_ent = msg.dict_find_int("piece");
			if (!type_ent || !piece_ent)
			{
#ifndef TORRENT_DISABLE_LOGGING
				m_pc.peer_log(peer_log_alert::incoming_message, peer_log_alert::ut_metadata
					, "missing or invalid keys");
#endif
				m_pc.disconnect(errors::invalid_metadata_message, operation_t::bittorrent, peer_connection_interface::peer_error);
				return true;
			}
			auto const type = msg_t(type_ent.int_value());
			auto const piece = static_cast<int>(piece_ent.int_value());

#ifndef TORRENT_DISABLE_LOGGING
			m_pc.peer_log(peer_log_alert::incoming_message, peer_log_alert::ut_metadata
				, "type: %d piece: %d", static_cast<int>(type), piece);
#endif

			switch (type)
			{
				case msg_t::request:
				{
					if (!m_torrent.valid_metadata()
						|| piece < 0 || piece >= (m_tp.metadata().size() + 16 * 1024 - 1) / (16 * 1024))
					{
#ifndef TORRENT_DISABLE_LOGGING
						if (m_pc.should_log(peer_log_alert::info))
						{
							m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
								, "have: %d invalid piece %d metadata size: %d"
								, int(m_torrent.valid_metadata()), piece
								, m_torrent.valid_metadata()
									? int(m_tp.metadata().size()) : 0);
						}
#endif
						write_metadata_packet(msg_t::dont_have, piece);
						return true;
					}
					if (m_pc.send_buffer_size() < send_buffer_limit)
						write_metadata_packet(msg_t::piece, piece);
					else if (m_incoming_requests.size() < max_incoming_requests)
						m_incoming_requests.push_back(piece);
					else
						write_metadata_packet(msg_t::dont_have, piece);
				}
				break;
				case msg_t::piece:
				{
					auto const i = std::find(m_sent_requests.begin()
						, m_sent_requests.end(), piece);

					// unwanted piece?
					if (i == m_sent_requests.end())
					{
#ifndef TORRENT_DISABLE_LOGGING
						m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
							, "UNWANTED / TIMED OUT");
#endif
						return true;
					}

					m_sent_requests.erase(i);
					auto const len = msg.data_section().size();
					auto const total_size = msg.dict_find_int_value("total_size", 0);
					m_tp.received_metadata(*this, body.subspan(len), piece, static_cast<int>(total_size));
					maybe_send_request();
				}
				break;
				case msg_t::dont_have:
				{
					m_request_limit = std::max(aux::time_now() + minutes(1), m_request_limit);
					auto const i = std::find(m_sent_requests.begin()
						, m_sent_requests.end(), piece);
					// unwanted piece?
					if (i == m_sent_requests.end()) return true;
					m_sent_requests.erase(i);
				}
				break;
			}

			m_pc.stats_counters().inc_stats_counter(counters::num_incoming_metadata);

			return true;
		}

		void tick() override
		{
			maybe_send_request();
			while (!m_incoming_requests.empty()
				&& m_pc.send_buffer_size() < send_buffer_limit)
			{
				int const piece = m_incoming_requests.front();
				m_incoming_requests.erase(m_incoming_requests.begin());
				write_metadata_packet(msg_t::piece, piece);
			}
		}

		void maybe_send_request()
		{
			if (m_pc.is_disconnecting()) return;

			// if we don't have any metadata, and this peer
			// supports the request metadata extension
			// and we aren't currently waiting for a request
			// reply. Then, send a request for some metadata.
			if (!m_torrent.valid_metadata()
				&& m_message_index != 0
				&& m_sent_requests.size() < 2
				&& has_metadata())
			{
				int const piece = m_tp.metadata_request(m_pc.has_metadata());
				if (piece == -1) return;

				m_sent_requests.push_back(piece);
				write_metadata_packet(msg_t::request, piece);
			}
		}

		bool has_metadata() const
		{
			return m_pc.has_metadata() || (aux::time_now() > m_request_limit);
		}

		void failed_hash_check(time_point const& now)
		{
			m_request_limit = now + seconds(20 + aux::random(50));
		}

		// explicitly disallow assignment, to silence msvc warning
		ut_metadata_peer_plugin& operator=(ut_metadata_peer_plugin const&) = delete;

	private:

		// this is the message index the remote peer uses
		// for metadata extension messages.
		int m_message_index = 0;

		// this is set to the next time we can request pieces
		// again. It is updated every time we get a
		// "I don't have metadata" message, but also when
		// we receive metadata that fails the info hash check
		time_point m_request_limit;

		// request queues
		std::vector<int> m_sent_requests;
		std::vector<int> m_incoming_requests;

		aux::torrent& m_torrent;
		aux::bt_peer_connection& m_pc;
		ut_metadata_plugin& m_tp;
	};

	std::shared_ptr<peer_plugin> ut_metadata_plugin::new_connection(
		peer_connection_handle const& pc)
	{
		if (pc.type() != connection_type::bittorrent) return {};

		aux::bt_peer_connection* c = static_cast<aux::bt_peer_connection*>(pc.native_handle().get());
		return std::make_shared<ut_metadata_peer_plugin>(m_torrent, *c, *this);
	}

	// has_metadata is false if the peer making the request has not announced
	// that it has metadata. In this case, it shouldn't prevent other peers
	// from requesting this block by setting a timeout on it.
	int ut_metadata_plugin::metadata_request(bool const has_metadata)
	{
		auto i = std::min_element(
			m_requested_metadata.begin(), m_requested_metadata.end());

		if (m_requested_metadata.empty())
		{
			// if we don't know how many pieces there are
			// just ask for piece 0
			m_requested_metadata.resize(1);
			i = m_requested_metadata.begin();
		}

		int const piece = int(i - m_requested_metadata.begin());

		// don't request the same block more than once every 3 seconds
		// unless the source is disconnected
		auto source = m_requested_metadata[piece].source.lock();
		time_point const now = aux::time_now();
		if (m_requested_metadata[piece].last_request != min_time()
			&& source
			&& !source->m_pc.is_disconnecting()
			&& total_seconds(now - m_requested_metadata[piece].last_request) < 3)
			return -1;

		++m_requested_metadata[piece].num_requests;

		// only set the timeout on this block, only if the peer
		// has metadata. This is to prevent peers with no metadata
		// to starve out sending requests to peers with metadata
		if (has_metadata)
			m_requested_metadata[piece].last_request = now;

		return piece;
	}

	bool ut_metadata_plugin::received_metadata(ut_metadata_peer_plugin& source
		, span<char const> buf, int const piece, int const total_size)
	{
		if (m_torrent.valid_metadata())
		{
#ifndef TORRENT_DISABLE_LOGGING
			source.m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
				, "already have metadata");
#endif
			m_torrent.add_redundant_bytes(static_cast<int>(buf.size()), aux::waste_reason::piece_unknown);
			return false;
		}

		if (m_metadata.empty())
		{
			// verify the total_size
			if (total_size <= 0 || total_size > m_torrent.session().settings().get_int(settings_pack::max_metadata_size))
			{
#ifndef TORRENT_DISABLE_LOGGING
				source.m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
					, "metadata size too big: %d", total_size);
#endif
// #error post alert
				return false;
			}

			m_metadata.resize(total_size);
			m_requested_metadata.resize(div_round_up(total_size, 16 * 1024));
		}

		if (piece < 0 || piece >= m_requested_metadata.end_index())
		{
#ifndef TORRENT_DISABLE_LOGGING
			source.m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
				, "piece: %d INVALID", piece);
#endif
			return false;
		}

		if (total_size != m_metadata.end_index())
		{
#ifndef TORRENT_DISABLE_LOGGING
			source.m_pc.peer_log(peer_log_alert::info, peer_log_alert::ut_metadata
				, "total_size: %d INCONSISTENT WITH: %d"
				, total_size, int(metadata().size()));
#endif
			// they disagree about the size!
			return false;
		}

		if (piece * 16 * 1024 + buf.size() > metadata().size())
		{
			// this piece is invalid
			return false;
		}

		std::memcpy(&m_metadata[piece * 16 * 1024], buf.data(), aux::numeric_cast<std::size_t>(buf.size()));
		// mark this piece has 'have'
		m_requested_metadata[piece].num_requests = std::numeric_limits<int>::max();
		m_requested_metadata[piece].source = source.shared_from_this();

		bool have_all = std::all_of(m_requested_metadata.begin(), m_requested_metadata.end()
			, [](metadata_piece const& mp) { return mp.num_requests == std::numeric_limits<int>::max(); });

		if (!have_all) return false;

		if (!m_torrent.set_metadata(m_metadata))
		{
			if (!m_torrent.valid_metadata())
			{
				time_point const now = aux::time_now();
				// any peer that we downloaded metadata from gets a random time
				// penalty, from 5 to 30 seconds or so. During this time we don't
				// make any metadata requests from those peers (to mix it up a bit
				// of which peers we use)
				// if we only have one block, and thus requested it from a single
				// peer, we bump up the retry time a lot more to try other peers
				bool single_peer = m_requested_metadata.size() == 1;
				for (auto& mp : m_requested_metadata)
				{
					mp.num_requests = 0;
					auto peer = mp.source.lock();
					if (!peer) continue;

					peer->failed_hash_check(single_peer ? now + minutes(5) : now);
				}
			}
			return false;
		}

		// free our copy of the metadata and get a reference
		// to the torrent's copy instead. No need to keep two
		// identical copies around
		m_metadata.clear();
		m_metadata.shrink_to_fit();

		// clear the storage for the bitfield
		m_requested_metadata.clear();
		m_requested_metadata.shrink_to_fit();

		return true;
	}

} }

namespace libtorrent {

	std::shared_ptr<torrent_plugin> create_ut_metadata_plugin(torrent_handle const& th, client_data_t)
	{
		aux::torrent* t = th.native_handle().get();
		// don't add this extension if the torrent is private
		if (t->valid_metadata() && t->torrent_file().priv()) return {};
		return std::make_shared<ut_metadata_plugin>(*t);
	}
}

#endif
