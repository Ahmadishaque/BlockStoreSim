#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace blockstoresim {

struct BlockMetadata {
    std::string hash;
    std::size_t size_bytes = 0;
    std::size_t ref_count = 0;
    bool reclaimable = false;
};

struct StoreStats {
    std::size_t logical_records = 0;
    std::size_t physical_blocks = 0;
    std::size_t logical_bytes = 0;
    std::size_t physical_bytes = 0;
    std::size_t duplicate_writes = 0;
    std::size_t overwrite_writes = 0;
    std::size_t deleted_records = 0;
    std::size_t reclaimable_blocks = 0;
    std::size_t reclaimable_bytes = 0;
};

struct SnapshotEntry {
    std::string key;
    std::string hash;
    std::size_t size_bytes = 0;
};

class BlockStore {
public:
    bool write(const std::string& key, const std::string& payload);
    std::optional<std::string> read(const std::string& key) const;
    bool erase(const std::string& key);
    std::size_t compact();

    StoreStats stats() const;
    std::vector<SnapshotEntry> snapshot() const;

    static std::string hash_payload(const std::string& payload);

private:
    void decrement_ref(const std::string& hash);

    std::unordered_map<std::string, std::string> key_to_hash_;
    std::unordered_map<std::string, std::string> block_payloads_;
    std::unordered_map<std::string, BlockMetadata> metadata_;

    std::size_t duplicate_writes_ = 0;
    std::size_t overwrite_writes_ = 0;
    std::size_t deleted_records_ = 0;
};

std::string format_stats(const StoreStats& stats);
std::string format_snapshot(const std::vector<SnapshotEntry>& snapshot);

} // namespace blockstoresim
