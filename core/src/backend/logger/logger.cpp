#include "./logger.hpp"
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace arc {

std::shared_ptr<Logger> loggerInstance;

void Logger::SetLogPreferences(std::string logFileName = "",
                               LogLevel level = LogLevel::ERROR,
                               LogOutput output = LogOutput::CONSOLE) {
  logLevel = level;
  logOutput = output;

  if (logOutput == LogOutput::FILE && !logFileName.empty()) {
    logFile.open(logFileName);
    if (logFile.good()) {
      std::cerr << "Can't Open Log File" << std::endl;
      logOutput = LogOutput::CONSOLE;
    }
  }
}

std::shared_ptr<Logger> Logger::GetInstance() {
  if (loggerInstance == nullptr) {
    loggerInstance = std::shared_ptr<Logger>(new Logger());
  }

  return loggerInstance;
}

void Logger::Log(std::string codeFile, int codeLine, std::string message,
                 LogLevel messageLevel = LogLevel::DEBUG) {
  if (messageLevel <= logLevel) {
    std::string logType;

    switch (messageLevel) {
    case LogLevel::DEBUG:
      logType = "DEBUG: ";
      break;
    case LogLevel::INFO:
      logType = "INFO: ";
      break;
    case LogLevel::WARN:
      logType = "WARN: ";
      break;
    case LogLevel::ERROR:
      logType = "ERROR: ";
      break;
    default:
      logType = "NONE: ";
      break;
    }
    codeFile += " : " + std::to_string(codeLine) + " : ";
    message = logType + codeFile + message;

    LogMessage(message);
  }
}

LogOutput Logger::GetLogOutput(const std::string &logOutput) {
  if (logOutput == "FILE") {
    return LogOutput::FILE;
  }

  return LogOutput::CONSOLE;
}

void Logger::LogMessage(const std::string &message) {
  if (logOutput == LogOutput::FILE) {
    logFile << message << std::endl;
  } else {
    std::cout << message << std::endl;
  }
}

} // namespace arc
