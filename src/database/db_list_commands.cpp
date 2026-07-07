#include <types/result.hpp>
#include <types/db_value.hpp>
#include <database/item.hpp>
#include <database/database.hpp>

#include <utility>
#include <variant>
#include <cstdint>
#include <optional>
#include <utility>
#include <algorithm>
#include <string>
#include <span>
#include <cstddef>
#include <vector>

Result DataBase::Lpush(
    const std::string& key,
    std::span<const std::string> values)
{
    RedisItem* item = FindItem(key);
    if (item != nullptr && std::get_if<ListValue>(&item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(ValuesMemoryUsage(values))) return OomError();

    ListValue* list = GetOrCreateValue<ListValue>(key);
    for (const std::string& value : values) {
        list->data.push_front(value);
    }

    return Result::Integer(static_cast<int64_t>(list->data.size()));
}

Result DataBase::Rpush(
    const std::string& key,
    std::span<const std::string> values)
{
    RedisItem* item = FindItem(key);
    if (item != nullptr && std::get_if<ListValue>(&item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(ValuesMemoryUsage(values))) return OomError();

    ListValue* list = GetOrCreateValue<ListValue>(key);
    for (const std::string& value : values) {
        list->data.push_back(value);
    }

    return Result::Integer(static_cast<int64_t>(list->data.size()));
}

Result DataBase::Llen(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    const auto* list = std::get_if<ListValue>(&item->value);
    if (list == nullptr) {
        return WrongTypeError();
    }

    return Result::Integer(static_cast<int64_t>(list->data.size()));
}

Result DataBase::Lindex(const std::string& key, int64_t index) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Nil();

    const auto* list = std::get_if<ListValue>(&item->value);
    if (list == nullptr) {
        return WrongTypeError();
    }

    auto normalized = NormalizeListIndex(index, list->data.size());
    if (!normalized.has_value()) {
        return Result::Nil();
    }

    return Result::String(list->data[normalized.value()]);
}

Result DataBase::Lset(const std::string& key, int64_t index, std::string value) {
    RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Error("no such key");

    auto* list = std::get_if<ListValue>(&item->value);
    if (list == nullptr) return WrongTypeError();

    auto normalized = NormalizeListIndex(index, list->data.size());
    if (!normalized.has_value()) {
        return Result::Error("index out of range");
    }

    const std::string& old_value = list->data[normalized.value()];
    if (value.size() > old_value.size() &&
        PossibleToAdd(value.size() - old_value.size())) {
        return OomError();
    }

    list->data[normalized.value()] = std::move(value);
    return Result::Done();
}

Result DataBase::Lpop(
    const std::string& key,
    std::optional<size_t> count)
{
    return PopList(key, count, true);
}

Result DataBase::Rpop(
    const std::string& key,
    std::optional<size_t> count)
{
    return PopList(key, count, false);
}

Result DataBase::Linsert(
    const std::string& key,
    bool insert_before,
    const std::string& pivot,
    std::string value)
{
    RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0); // doesn't exists

    auto* list = std::get_if<ListValue>(&item->value);
    if (list == nullptr) return WrongTypeError();

    auto pivot_pos = std::find(
        list->data.begin(),
        list->data.end(),
        pivot
    );

    if (pivot_pos == list->data.end()) {
        return Result::Integer(-1); // pivot wasn't found
    }

    auto insert_pos = pivot_pos;
    if (!insert_before) ++insert_pos;
    
    if (PossibleToAdd(sizeof(value) + value.size())) return OomError();

    list->data.insert(insert_pos, std::move(value));
    return Result::Integer(static_cast<int64_t>(list->data.size()));
}

Result DataBase::Lrange(const std::string& key, int64_t start, int64_t stop) const {
    auto item = FindItem(key);
    if (!item) return Result::Array({});

    const auto* list = std::get_if<ListValue>(&item->value);
    if (!list) return WrongTypeError();

    const int64_t size = static_cast<int64_t>(list->data.size());
    
    if (start < 0) start += size;
    if (stop < 0) stop += size;

    start = std::max<int64_t>(0, start);
    stop = std::min<int64_t>(size - 1, stop);

    if (start > stop || start >= size) {
        return Result::Array({});
    }

    auto begin = list->data.begin() + start;
    auto end = list->data.begin() + stop + 1;

    return Result::Array(std::vector<std::string>(begin, end));
}