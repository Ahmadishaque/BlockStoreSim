#include "block_store.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace blockstoresim {

namespace {
constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
}

std::string BlockStore::hash_payload(const std::string& payload) {
    std::uint64_t hash = FNV_OFFSET_BASIS;
    for (unsigned char ch : payload) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= FNV_PRIME;
    }

    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

bool BlockStore::write(const std::string& key, const std::string& payload) {
    const std::string new_hash = hash_payload(payload);

    const auto existing_key = key_to_hash_.find(key);
    if (existing_key != key_to_hash_.end()) {
        if (existing_key->second == new_hash) {
            return false;
        }
        overwrite_writes_++;
        decrement_ref(existing_key->second);
    }

    auto block_it = metadata_.find(new_hash);
    if (block_it == metadata_.end()) {
        metadata_[new_hash] = BlockMetadata{new_hash, payload.size(), 1, false};
        block_payloads_[new_hash] = payload;
    } else {
        block_it->second.ref_count++;
        block_it->second.reclaimable = false;
        duplicate_writes_++;
    }

    key_to_hash_[key] = new_hash;
    return true;
}

std::optional<std::string> BlockStore::read(const std::string& key) const {
    const auto key_it = key_to_hash_.find(key);
    if (key_it == key_to_hash_.end()) {
        return std::nullopt;
    }

    const auto payload_it = block_payloads_.find(key_it->second);
    if (payload_it == block_payloads_.end()) {
        return std::nullopt;
    }

    return payload_it->second;
}

bool BlockStore::erase(const std::string& key) {
    const auto key_it = key_to_hash_.find(key);
    if (key_it == key_to_hash_.end()) {
        return false;
    }

    decrement_ref(key_it->second);
    key_to_hash_.erase(key_it);
    deleted_records_++;
    return true;
}

std::size_t BlockStore::compact() {
    std::vector<std::string> hashes_to_remove;
    for (const auto& [hash, meta] : metadata_) {
        if (meta.ref_count == 0) {
            hashes_to_remove.push_back(hash);
        }
    }

    for (const auto& hash : hashes_to_remove) {
        metadata_.erase(hash);
        block_payloads_.erase(hash);
    }

    return hashes_to_remove.size();
}

StoreStats BlockStore::stats() const {
    StoreStats result;
    result.logical_records = key_to_hash_.size();
    result.physical_blocks = metadata_.size();
    result.duplicate_writes = duplicate_writes_;
    result.overwrite_writes = overwrite_writes_;
    result.deleted_records = deleted_records_;

    for (const auto& [_, hash] : key_to_hash_) {
        const auto meta_it = metadata_.find(hash);
        if (meta_it != metadata_.end()) {
            result.logical_bytes += meta_it->second.size_bytes;
        }
    }

    for (const auto& [_, meta] : metadata_) {
        result.physical_bytes += meta.size_bytes;
        if (meta.ref_count == 0) {
            result.reclaimable_blocks++;
            result.reclaimable_bytes += meta.size_bytes;
        }
    }

    return result;
}

std::vector<SnapshotEntry> BlockStore::snapshot() const {
    std::vector<SnapshotEntry> entries;
    entries.reserve(key_to_hash_.size());

    for (const auto& [key, hash] : key_to_hash_) {
        const auto meta_it = metadata_.find(hash);
        if (meta_it == metadata_.end()) {
            continue;
        }
        entries.push_back(SnapshotEntry{key, hash, meta_it->second.size_bytes});
    }

    std::sort(entries.begin(), entries.end(), [](const SnapshotEntry& a, const SnapshotEntry& b) {
        return a.key < b.key;
    });

    return entries;
}

void BlockStore::decrement_ref(const std::string& hash) {
    auto meta_it = metadata_.find(hash);
    if (meta_it == metadata_.end()) {
        return;
    }

    if (meta_it->second.ref_count > 0) {
        meta_it->second.ref_count--;
    }

    if (meta_it->second.ref_count == 0) {
        meta_it->second.reclaimable = true;
    }
}

std::string format_stats(const StoreStats& stats) {
    std::ostringstream out;
    out << "logical_records=" << stats.logical_records << '\n'
        << "physical_blocks=" << stats.physical_blocks << '\n'
        << "logical_bytes=" << stats.logical_bytes << '\n'
        << "physical_bytes=" << stats.physical_bytes << '\n'
        << "duplicate_writes=" << stats.duplicate_writes << '\n'
        << "overwrite_writes=" << stats.overwrite_writes << '\n'
        << "deleted_records=" << stats.deleted_records << '\n'
        << "reclaimable_blocks=" << stats.reclaimable_blocks << '\n'
        << "reclaimable_bytes=" << stats.reclaimable_bytes;
    return out.str();
}

std::string format_snapshot(const std::vector<SnapshotEntry>& snapshot) {
    std::ostringstream out;
    for (const auto& entry : snapshot) {
        out << entry.key << " hash=" << entry.hash << " size=" << entry.size_bytes << '\n';
    }
    return out.str();
}

} // namespace blockstoresim
