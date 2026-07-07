#include <types/result.hpp>
#include <database/item.hpp>
#include <database/database.hpp>

#include <utility>
#include <variant>
#include <cstdint>
#include <regex>
#include <utility>
#include <chrono>
#include <cstddef>
#include <string>
#include <type_traits>
#include <span> 
#include <vector>

Result DataBase::Type(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) {
        return Result::SimpleString("none");
    }

    return std::visit(
        [](const auto& value) -> Result {
            using ValueType = std::remove_cvref_t<decltype(value)>;
            return Result::SimpleString(std::string(ValueType::kTypeName));
        },
        item->value
    );
}

Result DataBase::Exists(std::span<const std::string> keys) const {
    int64_t found_count = 0;

    for (const std::string& key : keys) {
        if (FindItem(key) != nullptr) ++found_count;
    }

    return Result::Integer(found_count);
}

Result DataBase::Del(std::span<const std::string> keys) {
    int64_t removed_count = 0;

    for (const std::string& key : keys) {
        auto erased_count = data_.erase(key);
        removed_count += static_cast<int64_t>(erased_count);
    }

    return Result::Integer(removed_count);
}

Result DataBase::DbSize() const {
    for (auto it = data_.begin(); it != data_.end();) {
        if (IsExpired(it->second)) it = data_.erase(it);
        else ++it;
    }

    return Result::Integer(static_cast<int64_t>(data_.size()));
}

Result DataBase::FlushDb() {
    data_.clear();
    return Result::Done();
}

Result DataBase::Keys(const std::string& pattern) const {
    std::regex regex_pattern(MakeRegexPattern(pattern));

    std::vector<std::string> result;
    for (const auto& [key, item] : data_) {
        if (std::regex_match(key, regex_pattern) && !IsExpired(item)) {
            result.push_back(key);
        }
    }

    return Result::Array(std::move(result));
}

Result DataBase::Expire(const std::string& key, int64_t seconds) {
    RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    if (seconds <= 0) {
        data_.erase(key);
        return Result::Integer(1);
    }

    item->expired_at =
        std::chrono::steady_clock::now() + std::chrono::seconds(seconds);

    return Result::Integer(1);
}

Result DataBase::Ttl(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(-2);

    if (!item->expired_at.has_value()) {
        return Result::Integer(-1);
    }

    auto now = std::chrono::steady_clock::now();
    if (item->expired_at.value() <= now) {
        data_.erase(key);
        return Result::Integer(-2);
    }

    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        item->expired_at.value() - now
    ).count();

    return Result::Integer(static_cast<int64_t>(remaining));
}

Result DataBase::MemoryUsage(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Nil();

    return Result::Integer(static_cast<int64_t>(ItemMemoryUsage(key, *item)));
}

Result DataBase::ConfigSetMaxMemory(size_t bytes) {
    SetMaxMemory(bytes);
    return Result::Done();
}

Result DataBase::ConfigGetMaxMemory() const {
    return Result::Integer(static_cast<int64_t>(max_memory_));
}

void DataBase::SetMaxMemory(size_t bytes) {
    max_memory_ = bytes;
}