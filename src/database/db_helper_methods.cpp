#include <types/result.hpp>
#include <types/db_value.hpp>
#include <database/item.hpp>
#include <database/database.hpp>

#include <utility>
#include <optional>
#include <string>
#include <cstddef>
#include <chrono>
#include <utility>
#include <variant>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <span>

const RedisItem* DataBase::FindItem(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return nullptr;
    }

    if (IsExpired(it->second)) {
        data_.erase(it);
        return nullptr;
    }

    return &it->second;
}

RedisItem* DataBase::FindItem(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return nullptr;
    }

    if (IsExpired(it->second)) {
        data_.erase(it);
        return nullptr;
    }

    return &it->second;
}

bool DataBase::IsExpired(const RedisItem& item) {
    return item.expired_at.has_value() &&
           std::chrono::steady_clock::now() >= item.expired_at.value();
}

std::string DataBase::MakeRegexPattern(const std::string& pattern) {
    std::string result;
    result.reserve(pattern.size() * 3);

    result.push_back('^');

    for (char ch : pattern) {
        if (ch == '*') {
            result += ".*";
        } else if (ch == '?') {
            result.push_back('.');
        } else if (
            ch == '.' || ch == '^' || ch == '$' || ch == '|' ||
            ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
            ch == '{' || ch == '}' || ch == '+' || ch == '\\'
        ) {
            result.push_back('\\');
            result.push_back(ch);
        } else {
            result.push_back(ch);
        }
    }

    result.push_back('$');
    return result;
}

std::optional<size_t> DataBase::NormalizeListIndex(int64_t index, size_t size) {
    if (size == 0) return std::nullopt;
    int64_t real_index = index;

    if (real_index < 0) {
        real_index += static_cast<int64_t>(size);
    }

    if (real_index < 0 || real_index >= static_cast<int64_t>(size)) {
        return std::nullopt;
    }

    return static_cast<size_t>(real_index);
}

Result DataBase::PopList(
    const std::string& key,
    std::optional<size_t> count,
    bool from_front)
{
    RedisItem* item = FindItem(key);
    if (item == nullptr) {
        return count.has_value() ? Result::Array({}) : Result::Nil();
    }

    auto* list = std::get_if<ListValue>(&item->value);
    if (list == nullptr) return WrongTypeError();

    if (!count.has_value()) {
        std::string value = PopOneValue(*list, from_front);
        EraseListIfEmpty(key, *list);
        return Result::String(std::move(value));
    }

    std::vector<std::string> values =
        PopManyValues(*list, count.value(), from_front);

    EraseListIfEmpty(key, *list);
    return Result::Array(std::move(values));
}

std::string DataBase::PopOneValue(ListValue& list, bool from_front) {
    std::string value;

    if (from_front) {
        value = std::move(list.data.front());
        list.data.pop_front();
    } else {
        value = std::move(list.data.back());
        list.data.pop_back();
    }

    return value;
}

std::vector<std::string> DataBase::PopManyValues(
    ListValue& list,
    size_t count,
    bool from_front)
{
    std::vector<std::string> values;
    size_t real_count = std::min(count, list.data.size());

    for (size_t i = 0; i < real_count; ++i) {
        values.push_back(PopOneValue(list, from_front));
    }

    return values;
}

void DataBase::EraseListIfEmpty(
    const std::string& key,
    const ListValue& list)
{
    if (list.data.empty()) {
        data_.erase(key);
    }
}

size_t DataBase::ItemMemoryUsage(
    const std::string& key,
    const RedisItem& item)
{
    size_t res = sizeof(RedisItem);
    res += sizeof(std::string) + key.capacity();
    res += sizeof(item.expired_at);
    res += std::visit(
        [](const auto& value) {
            return value.MemoryUsage();
        },
        item.value
    );

    return res;
}

size_t DataBase::ValuesMemoryUsage(std::span<const std::string> values) {
    size_t res = 0;
    for (const std::string& value : values) {
        res += sizeof(value) + value.capacity();
    }
    return res;
}

size_t DataBase::UsedMemory() const {
    size_t res = sizeof(DataBase);

    for (auto it = data_.begin(); it != data_.end();) {
        if (IsExpired(it->second)) {
            it = data_.erase(it);
            continue;
        }

        res += ItemMemoryUsage(it->first, it->second);
        ++it;
    }

    return res;
}

bool DataBase::PossibleToAdd(size_t bytes) const {
    if (max_memory_ == 0) return false;
    if (bytes == 0) return false;

    return bytes > max_memory_ || UsedMemory() > max_memory_ - bytes;
}

Result DataBase::OomError() {
    return Result::Error("OOM command not allowed when used memory > 'maxmemory'");
}

Result DataBase::WrongTypeError() {
    return Result::Error("wrong type");
}