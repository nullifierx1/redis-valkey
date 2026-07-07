#include <database/item.hpp>
#include <types/db_value.hpp>
#include <types/result.hpp>
#include <database/database.hpp>

#include <string>
#include <variant>
#include <algorithm>
#include <iterator>
#include <vector>
#include <span>
#include <cstdint>
#include <utility>
#include <set>

Result DataBase::Sadd(
    const std::string& key, 
    std::span<const std::string> values) 
{
    RedisItem* item = FindItem(key);
    if (item != nullptr && std::get_if<SetValue>(&item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(ValuesMemoryUsage(values))) return OomError();

    SetValue* set = GetOrCreateValue<SetValue>(key);
    int64_t added_count = 0;
    for (const std::string& value : values) {
        if (set->data.emplace(value).second) ++added_count;
    }

    return Result::Integer(added_count);
}

Result DataBase::Smembers(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Array({});

    const auto* set = std::get_if<SetValue>(&item->value);
    if (set == nullptr) return WrongTypeError();

    return Result::Array(std::vector<std::string>(set->data.begin(), set->data.end()));
}

Result DataBase::Scard(const std::string& key) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    const auto* set = std::get_if<SetValue>(&item->value);
    if (set == nullptr) return WrongTypeError();

    return Result::Integer(static_cast<int64_t>(set->data.size()));
}

Result DataBase::Sismember(const std::string& key, const std::string& member) const {
    const RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    const auto* set = std::get_if<SetValue>(&item->value);
    if (set == nullptr) return WrongTypeError();

    return Result::Integer(set->data.contains(member));
}

Result DataBase::Srem(const std::string& key, std::span<const std::string> values) {
    RedisItem* item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    auto* set = std::get_if<SetValue>(&item->value);
    if (set == nullptr) return WrongTypeError();

    int64_t removed_count = 0;
    for (const std::string& member : values) {
        removed_count += static_cast<int64_t>(set->data.erase(member));
    }

    if (set->data.empty()) data_.erase(key);
    return Result::Integer(removed_count);
}

Result DataBase::Smove(
    const std::string& source, 
    const std::string& destination, 
    const std::string& member
) {
    RedisItem* item1 = FindItem(source);
    if (item1 == nullptr) return Result::Integer(0);

    auto* set1 = std::get_if<SetValue>(&item1->value);
    if (set1 == nullptr) return WrongTypeError();

    if (!set1->data.contains(member)) {
        return Result::Integer(0);
    }

    RedisItem* item2 = FindItem(destination);
    auto* set2 = item2 == nullptr ? nullptr : std::get_if<SetValue>(&item2->value);
    if (item2 != nullptr && set2 == nullptr) return WrongTypeError();

    if ((set2 == nullptr || !set2->data.contains(member)) &&
        PossibleToAdd(sizeof(member) + member.size())) {
        return OomError();
    }

    set2 = GetOrCreateValue<SetValue>(destination);
    set1->data.erase(member);
    set2->data.emplace(member);
    
    return Result::Integer(1);
}

Result DataBase::Sunion(std::span<const std::string> keys) const {   
    std::set<std::string> result;
    
    for (const auto key2 : keys) {
        auto item2 = FindItem(key2);
        if (item2 == nullptr) continue;

        auto union_item = std::get_if<SetValue>(&item2->value);
        if (union_item == nullptr) return WrongTypeError();

        result.insert(union_item->data.begin(), union_item->data.end());
    }

    return Result::Array(std::vector<std::string>(result.begin(), result.end()));
}

Result DataBase::Sdiff(const std::string& key, std::span<const std::string> another_keys) const {
    auto item1 = FindItem(key);
    if (item1 == nullptr) return Result::Array({});

    auto* origin = std::get_if<SetValue>(&item1->value);
    if (origin == nullptr) return WrongTypeError();

    std::set<std::string> result(origin->data.begin(), origin->data.end());

    for (const auto key2 : another_keys) {
        auto item2 = FindItem(key2);
        if (item2 == nullptr) continue;

        auto subtract = std::get_if<SetValue>(&item2->value);
        if (subtract == nullptr) return WrongTypeError();

        std::set<std::string> next_result;
        std::set_difference(
            result.begin(), result.end(),
            subtract->data.begin(), subtract->data.end(),
            std::inserter(next_result, next_result.begin())
        );
        result = std::move(next_result);
    }

    return Result::Array(std::vector<std::string>(result.begin(), result.end()));
}

Result DataBase::Sinter(const std::string& key, std::span<const std::string> another_keys) const {
    auto item1 = FindItem(key);
    if (item1 == nullptr) return Result::Array({});

    auto* origin = std::get_if<SetValue>(&item1->value);
    if (origin == nullptr) return WrongTypeError();

    std::set<std::string> result(origin->data.begin(), origin->data.end());

    for (const auto key2 : another_keys) {
        auto item2 = FindItem(key2);
        if (item2 == nullptr) return Result::Array({});

        auto inter_item = std::get_if<SetValue>(&item2->value);
        if (inter_item == nullptr) return WrongTypeError();

        std::set<std::string> next_result;
        std::set_intersection(
            result.begin(), result.end(),
            inter_item->data.begin(), inter_item->data.end(),
            std::inserter(next_result, next_result.begin())
        );
        result = std::move(next_result);
    }

    return Result::Array(std::vector<std::string>(result.begin(), result.end()));   
}