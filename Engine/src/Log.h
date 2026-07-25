#pragma once
#include "archpch.h"

namespace Engine {
	enum class LogLevel {
		Trace,
		Info, 
		Warn,
		Error 
	};

	class Log {
	public:
		static void Print(LogLevel level, const char* fmt, ...);
	};

	const char* LogLevelToString(LogLevel level);
}

#define LOG_TRACE(...) ::Engine::Log::Print(::Engine::LogLevel::Trace, __VA_ARGS__)
#define LOG_INFO(...)  ::Engine::Log::Print(::Engine::LogLevel::Info,  __VA_ARGS__)
#define LOG_WARN(...)  ::Engine::Log::Print(::Engine::LogLevel::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) ::Engine::Log::Print(::Engine::LogLevel::Error, __VA_ARGS__)