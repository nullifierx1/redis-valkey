#pragma once

#include <types/db_value.hpp>
#include <types/result.hpp>
#include <database/item.hpp>

#include <string>
#include <unordered_map>
#include <optional>
#include <span>
#include <cstddef>
#include <cstdint>
#include <utility>

class DataBase {
public:
    /* string methods */
    Result Set(std::string key, std::string value);
    Result Get(const std::string& key) const;
    Result Strlen(const std::string& key) const;
    Result Append(const std::string& key, std::string value);

    /* common methods */
    Result Exists(std::span<const std::string> keys) const;
    Result Type(const std::string& key) const;
    Result Del(std::span<const std::string> keys);
    Result DbSize() const;
    Result FlushDb();
    Result Keys(const std::string& pattern) const;
    Result Expire(const std::string& key, int64_t seconds);
    Result Ttl(const std::string& key) const;
    Result MemoryUsage(const std::string& key) const;
    Result ConfigSetMaxMemory(size_t bytes);
    Result ConfigGetMaxMemory() const;

    /* list methods */
    Result Lpush(const std::string& key, std::span<const std::string> values);
    Result Rpush(const std::string& key, std::span<const std::string> values);
    Result Llen(const std::string& key) const;
    Result Lindex(const std::string& key, int64_t index) const;
    Result Lset(const std::string& key, int64_t index, std::string value);
    Result Lpop(const std::string& key, std::optional<size_t> count);
    Result Rpop(const std::string& key, std::optional<size_t> count);
    Result Linsert(const std::string& key, bool is_before, const std::string& pivot, std::string value);
    Result Lrange(const std::string& key, int64_t start, int64_t stop) const;

    /* set methods */
    Result Sadd(const std::string& key, std::span<const std::string> values);
    Result Smembers(const std::string& key) const;
    Result Scard(const std::string& key) const;
    Result Sismember(const std::string& key, const std::string& member) const;
    Result Srem(const std::string& key, std::span<const std::string> values);
    Result Smove(const std::string& source, const std::string& destination, const std::string& member);
    Result Sunion(std::span<const std::string> another_keys) const ;
    Result Sdiff(const std::string& key, std::span<const std::string> another_keys) const;
    Result Sinter(const std::string& key, std::span<const std::string> another_keys) const;

    /* geo methods */
    Result Geoadd(
        const std::string& key, 
        std::span<const std::string> locations
    );

    Result Geopos(const std::string& key, std::span<const std::string> locations) const;
    Result Geodist(
        const std::string& key,
        const std::string& member1,
        const std::string& member2,
        const std::string& unit
    ) const;

    Result Geosearch(const std::string& key, std::span<const std::string> args) const;
    Result Geosearchstore(const std::string& dest, const std::string& source, std::span<const std::string> args);

    /* other methods */
    void SetMaxMemory(size_t bytes);

private:
    friend class DataBaseTestAccess;

    /* internal helper-methods */
    const RedisItem* FindItem(const std::string& key) const;
    RedisItem* FindItem(const std::string& key);

    Result PopList(
        const std::string& key,
        std::optional<size_t> count,
        bool from_front
    );

    static std::string PopOneValue(ListValue& list, bool from_front);
    static std::vector<std::string> PopManyValues(
        ListValue& list, size_t count, bool from_front
    );

    void EraseListIfEmpty(const std::string& key, const ListValue& list);

    template <typename T>
    T* GetOrCreateValue(const std::string& key) {
        auto it = data_.find(key);
        if (it != data_.end() && IsExpired(it->second)) {
            data_.erase(it);
            it = data_.end();
        }
        if (it == data_.end()) {
            RedisItem item(Value{T{}});
            it = data_.emplace(key, std::move(item)).first;
        }
        return std::get_if<T>(&it->second.value);
    }

    static std::string MakeRegexPattern(const std::string& pattern);
    static std::optional<size_t> NormalizeListIndex(int64_t index, size_t size);
    static bool IsExpired(const RedisItem& item);
    static size_t ItemMemoryUsage(const std::string& key, const RedisItem& item);
    static size_t ValuesMemoryUsage(std::span<const std::string> values);

    size_t UsedMemory() const;
    bool PossibleToAdd(size_t bytes) const;

    /* command execution errors */
    static Result OomError();
    static Result WrongTypeError();

    mutable std::unordered_map<std::string, RedisItem> data_;
    size_t max_memory_ = 0;
};
