#include "block_store.hpp"

#include <iostream>
#include <sstream>
#include <string>

using blockstoresim::BlockStore;
using blockstoresim::format_snapshot;
using blockstoresim::format_stats;

namespace {

void print_help() {
    std::cout
        << "Commands:\n"
        << "  WRITE <key> <payload>   Store or overwrite a logical record\n"
        << "  READ <key>              Print payload for key\n"
        << "  DELETE <key>            Delete logical key\n"
        << "  STATS                   Print storage counters\n"
        << "  COMPACT                 Remove unreferenced physical blocks\n"
        << "  SNAPSHOT                Print deterministic key/hash/size metadata\n"
        << "  HELP                    Show this message\n"
        << "  EXIT                    Stop the process\n";
}

std::string rest_of_line(std::istringstream& stream) {
    std::string payload;
    std::getline(stream, payload);
    if (!payload.empty() && payload.front() == ' ') {
        payload.erase(payload.begin());
    }
    return payload;
}

} // namespace

int main() {
    BlockStore store;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command == "WRITE") {
            std::string key;
            input >> key;
            std::string payload = rest_of_line(input);

            if (key.empty() || payload.empty()) {
                std::cout << "ERR WRITE requires <key> and <payload>\n";
                continue;
            }

            const bool changed = store.write(key, payload);
            std::cout << (changed ? "OK write" : "OK no-op") << " key=" << key << '\n';
        } else if (command == "READ") {
            std::string key;
            input >> key;
            const auto payload = store.read(key);
            if (!payload.has_value()) {
                std::cout << "ERR missing key=" << key << '\n';
            } else {
                std::cout << "VALUE key=" << key << " payload=" << *payload << '\n';
            }
        } else if (command == "DELETE") {
            std::string key;
            input >> key;
            const bool removed = store.erase(key);
            std::cout << (removed ? "OK delete" : "ERR missing") << " key=" << key << '\n';
        } else if (command == "STATS") {
            std::cout << format_stats(store.stats()) << '\n';
        } else if (command == "COMPACT") {
            const std::size_t removed = store.compact();
            std::cout << "OK compact removed_blocks=" << removed << '\n';
        } else if (command == "SNAPSHOT") {
            const auto snapshot = store.snapshot();
            if (snapshot.empty()) {
                std::cout << "SNAPSHOT empty\n";
            } else {
                std::cout << format_snapshot(snapshot);
            }
        } else if (command == "HELP") {
            print_help();
        } else if (command == "EXIT") {
            break;
        } else {
            std::cout << "ERR unknown command=" << command << '\n';
        }
    }

    return 0;
}
