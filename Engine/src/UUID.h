#pragma once
#include "archpch.h"
#include <iomanip>
#include <format>

namespace Engine::Util {
    class UUID {
    public:
        UUID(uint64_t uuid) : _Value(uuid) {}
        UUID() : _Value(s_UniformDistribution(s_Engine)) {}
        UUID(const UUID&) = default;

        explicit operator uint64_t() const { return _Value; }

        [[nodiscard]] std::string ToString() const {
            return std::format("{:016x}", _Value);
        }

        bool operator==(const UUID& other) const { return _Value == other._Value; }
        bool operator!=(const UUID& other) const { return _Value != other._Value; }
        bool operator<(const UUID& other) const { return _Value < other._Value; }

    private:
        uint64_t _Value;
        static inline std::random_device s_RandomDevice;
        static inline std::mt19937_64 s_Engine{ s_RandomDevice() };
        static inline std::uniform_int_distribution<uint64_t> s_UniformDistribution;
    };
}

namespace std {
    template<>
    struct hash<Engine::Util::UUID> {
        size_t operator()(const Engine::Util::UUID& uuid) const {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };
}