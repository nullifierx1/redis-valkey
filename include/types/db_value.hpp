#pragma once

#include <cstddef>
#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

struct StringValue {
    static constexpr std::string_view kTypeName = "string";
    
    size_t MemoryUsage() const {
        return sizeof(StringValue) + data.capacity();
    }
    
    std::string data;
};

struct ListValue {
    static constexpr std::string_view kTypeName = "list";

    size_t MemoryUsage() const {
        size_t res = sizeof(ListValue);
        for (const auto& item : data) {
            res += sizeof(item) + item.capacity();
        }
        return res;
    }

    std::deque<std::string> data;
};

struct SetValue {
    static constexpr std::string_view kTypeName = "set";

    size_t MemoryUsage() const {
        size_t res = sizeof(SetValue);
        for (const auto& item : data) {
            res += sizeof(item) + item.capacity();
        }
        return res;
    }

    std::set<std::string> data;
};

struct GeoValue {
    static constexpr std::string_view kTypeName = "geo";
    using coordinates_type = long double;

    size_t MemoryUsage() const {
        size_t res = sizeof(GeoValue);
        for (const auto& [member, loc] : data) {
            res += sizeof(member) + member.capacity();
            res += sizeof(loc) + loc.member.capacity();
        }
        return res;
    }

    struct Location {
        coordinates_type latitude;
        coordinates_type longitude;
        std::string member;
    };

    std::unordered_map<std::string, Location> data; 
};

using Value = std::variant<StringValue, ListValue, SetValue, GeoValue>;