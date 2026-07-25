#include "archpch.h"
#include "Log.h"
#include <cstdarg>

namespace Engine {

    const char* LogLevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        }
        return "UNKNOWN";
    }

    void Log::Print(LogLevel level, const char* fmt, ...) {
        char formatted[1024];

        va_list args;
        va_start(args, fmt);
        vsnprintf(formatted, sizeof(formatted), fmt, args);
        va_end(args);

        char buffer[1200];
        snprintf(buffer, sizeof(buffer), "[%s] %s\n", LogLevelToString(level), formatted);

        OutputDebugStringA(buffer);   // shows in Visual Studio's Output window
        std::fputs(buffer, stdout);   // also shows in console if you have one attached
    }
}