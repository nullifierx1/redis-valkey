#include <types/result.hpp>
#include <types/db_value.hpp>
#include <database/item.hpp>
#include <database/database.hpp>

#include <utility>
#include <variant>
#include <cstdint>
#include <cstddef>
#include <string>

Result DataBase::Set(std::string key, std::string value) {
    RedisItem item(Value{StringValue{std::move(value)}});
    const size_t new_usage = ItemMemoryUsage(key, item);
    const RedisItem* old_item = FindItem(key);
    const size_t old_usage = old_item == nullptr ? 0 : ItemMemoryUsage(key, *old_item);
    if (new_usage > old_usage && PossibleToAdd(new_usage - old_usage)) {
        return OomError();
    }

    data_.insert_or_assign(std::move(key), std::move(item));
    return Result::Done();
}

Result DataBase::Get(const std::string& key) const {
    auto target = FindItem(key);
    if (!target) return Result::Nil();

    const auto* value = std::get_if<StringValue>(&target->value);
    if (value == nullptr) return WrongTypeError();

    return Result::String(value->data);
}

Result DataBase::Strlen(const std::string& key) const {
    auto item = FindItem(key);
    if (item == nullptr) return Result::Integer(0);

    const auto* value = std::get_if<StringValue>(&item->value);
    if (value == nullptr) return WrongTypeError();

    return Result::Integer(static_cast<int64_t>(value->data.size()));
}

Result DataBase::Append(const std::string& key, std::string value) {
    RedisItem* item = FindItem(key);
    if (item != nullptr && std::get_if<StringValue>(&item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(value.size())) return OomError();

    StringValue* string_value = GetOrCreateValue<StringValue>(key);
    string_value->data.append(value);
    return Result::Integer(static_cast<int64_t>(string_value->data.size()));
}