
// This header is generated by tools/gen_convenience_header.py

#include "libtorrent/add_torrent_params.hpp"
#include "libtorrent/address.hpp"
#include "libtorrent/alert.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/announce_entry.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/bdecode.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/bitfield.hpp"
#include "libtorrent/client_data.hpp"
#include "libtorrent/close_reason.hpp"
#include "libtorrent/config.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/disabled_disk_io.hpp"
#include "libtorrent/disk_buffer_holder.hpp"
#include "libtorrent/disk_interface.hpp"
#include "libtorrent/disk_observer.hpp"
#include "libtorrent/download_priority.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/error.hpp"
#include "libtorrent/error_code.hpp"
#include "libtorrent/extensions.hpp"
#include "libtorrent/extensions/i2p_pex.hpp"
#include "libtorrent/extensions/smart_ban.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/extensions/ut_pex.hpp"
#include "libtorrent/file_layout.hpp"
#include "libtorrent/file_storage.hpp"
#include "libtorrent/fingerprint.hpp"
#include "libtorrent/flags.hpp"
#include "libtorrent/fwd.hpp"
#include "libtorrent/gzip.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/hex.hpp"
#include "libtorrent/i2p_stream.hpp"
#include "libtorrent/identify_client.hpp"
#include "libtorrent/index_range.hpp"
#include "libtorrent/info_hash.hpp"
#include "libtorrent/io_context.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/kademlia/announce_flags.hpp"
#include "libtorrent/kademlia/dht_observer.hpp"
#include "libtorrent/kademlia/dht_settings.hpp"
#include "libtorrent/kademlia/dht_state.hpp"
#include "libtorrent/kademlia/dht_storage.hpp"
#include "libtorrent/kademlia/dht_tracker.hpp"
#include "libtorrent/kademlia/direct_request.hpp"
#include "libtorrent/kademlia/dos_blocker.hpp"
#include "libtorrent/kademlia/ed25519.hpp"
#include "libtorrent/kademlia/find_data.hpp"
#include "libtorrent/kademlia/get_item.hpp"
#include "libtorrent/kademlia/get_peers.hpp"
#include "libtorrent/kademlia/io.hpp"
#include "libtorrent/kademlia/item.hpp"
#include "libtorrent/kademlia/msg.hpp"
#include "libtorrent/kademlia/node.hpp"
#include "libtorrent/kademlia/node_entry.hpp"
#include "libtorrent/kademlia/node_id.hpp"
#include "libtorrent/kademlia/observer.hpp"
#include "libtorrent/kademlia/put_data.hpp"
#include "libtorrent/kademlia/refresh.hpp"
#include "libtorrent/kademlia/routing_table.hpp"
#include "libtorrent/kademlia/rpc_manager.hpp"
#include "libtorrent/kademlia/sample_infohashes.hpp"
#include "libtorrent/kademlia/traversal_algorithm.hpp"
#include "libtorrent/kademlia/types.hpp"
#include "libtorrent/load_torrent.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/mmap_disk_io.hpp"
#include "libtorrent/natpmp.hpp"
#include "libtorrent/operations.hpp"
#include "libtorrent/peer_class.hpp"
#include "libtorrent/peer_class_type_filter.hpp"
#include "libtorrent/peer_connection_handle.hpp"
#include "libtorrent/peer_connection_interface.hpp"
#include "libtorrent/peer_id.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/peer_request.hpp"
#include "libtorrent/performance_counters.hpp"
#include "libtorrent/pex_flags.hpp"
#include "libtorrent/piece_block.hpp"
#include "libtorrent/portmap.hpp"
#include "libtorrent/posix_disk_io.hpp"
#include "libtorrent/random.hpp"
#include "libtorrent/read_resume_data.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/session_handle.hpp"
#include "libtorrent/session_params.hpp"
#include "libtorrent/session_settings.hpp"
#include "libtorrent/session_stats.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/session_types.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/socket.hpp"
#include "libtorrent/socket_type.hpp"
#include "libtorrent/socks5_stream.hpp"
#include "libtorrent/span.hpp"
#include "libtorrent/storage_defs.hpp"
#include "libtorrent/string_view.hpp"
#include "libtorrent/time.hpp"
#include "libtorrent/torrent_flags.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/tracker_event.hpp"
#include "libtorrent/truncate.hpp"
#include "libtorrent/units.hpp"
#include "libtorrent/upnp.hpp"
#include "libtorrent/version.hpp"
#include "libtorrent/web_seed_entry.hpp"
#include "libtorrent/write_resume_data.hpp"
