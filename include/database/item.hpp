#pragma once

#include <types/db_value.hpp>

#include <chrono>
#include <optional>

class RedisItem {
public:
    RedisItem() = default;
    explicit RedisItem(Value value) : value(std::move(value)) {}

    Value value;
    std::optional<std::chrono::steady_clock::time_point> expired_at;
};