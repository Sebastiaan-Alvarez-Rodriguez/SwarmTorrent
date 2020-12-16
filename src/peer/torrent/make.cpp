#include <vector>

#include "peer/connection/protocol/tracker/connections.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/util/print.h"
#include "shared/util/fs/fs.h"
#include "torrent.h"

bool torrent::make(const std::string& in, const std::string& out, std::vector<std::string>& trackers) {
    if (!fs::is_file(in)) {
        std::cerr << print::RED << "[ERROR] No file found at '" << in << '\'' << print::CLEAR << '\n';
        return false;
    }
    if (fs::file_size(in) == 0) {
        std::cerr << print::RED << "[ERROR] File '" << in << "' is empty!" << print::CLEAR << '\n';
        return false;
    }
    auto t_size = trackers.size();
    if (t_size == 0) {
        std::cerr << print::RED << "[ERROR] Cannot make torrentfile without any trackers given" << print::CLEAR << '\n';
        return false;
    }
    IPTable table = IPTable::from(trackers);
    TorrentFile tf = TorrentFile::make_for(table, in);
    tf.save(out);
    const std::string torrent_hash = tf.getMetadata().content_hash;
    if (torrent_hash == "") {
        std::cerr << print::RED << "[ERROR] TorrentFile could not be hashed properly" << print::CLEAR << '\n';
        return false;
    }


    IPTable connected;
    for (auto it = table.cbegin(); it != table.cend(); ++it) {
        const auto addr = *it;

        auto conn = TCPClientConnection::Factory::from(NetType::IPv4).withAddress(addr.ip).withDestinationPort(addr.port).create();
        if (conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection to tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!conn->doConnect()) {
            std::cerr << print::YELLOW << "[WARN] Could not connect to tracker " << print::CLEAR;conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!connections::tracker::send::make_torrent(conn, torrent_hash)) {
            std::cerr << print::YELLOW << "[WARN] Could not send torrent request for tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }

        message::standard::Header h;
        if (message::standard::recv(conn, h) && h.formatType == message::standard::OK) {
            connected.add(addr);
        } else {
            std::cerr << print::YELLOW << "[WARN] No confirming message received from tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
    if (connected.size() == 0) {
        std::cerr << print::RED << "[ERROR] No successful connections to any tracker" << print::CLEAR << '\n';
        return false;
    } else if (connected.size() < t_size) {
        std::cerr << print::YELLOW << "[WARN] Could only register at "<< connected.size() << '/' << t_size << " trackers" << print::CLEAR << '\n';
    } else {
        std::cerr << print::GREEN << "[SUCCESS] Registered torrentfile-in-progress at " << connected.size() << '/' << t_size << " trackers" << print::CLEAR << '\n';
    }
    return true;
}
