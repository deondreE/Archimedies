#pragma once
#include "archpch.h"
#include <iomanip>

namespace Engine::Util {
    class UUID {
    public:
        UUID() : _Value(s_UniformDistribution(s_Engine)) {}
        operator uint64_t() const { return _Value; }

        [[nodiscard]] std::string ToString() const {
            std::stringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(16) << _Value;
            return ss.str();
        }
    private:
        uint64_t _Value;
        static inline std::random_device s_RandomDevice;
        static inline std::mt19937_64 s_Engine{ s_RandomDevice() };
        static inline std::uniform_int_distribution<uint64_t> s_UniformDistribution;
    };
}