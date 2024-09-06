#pragma once

#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <sstream>

namespace atlantis::logging {

class Logger;

enum class Level : uint8_t {
  ERR = 0,
  WARN = 1,
  INFO = 2,
  DEBUG = 3,
  TRACE = 4
};

class LogScopeWrapper {
 public:
  virtual ~LogScopeWrapper() = default;

  virtual void begin(Logger& logger, Level level) = 0;
  virtual void end(Logger& logger, Level level) = 0;
};

class TimedLogScopeWrapper : public LogScopeWrapper {
 private:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = Clock::time_point;

  const char* _title;
  TimePoint _startTime;

 public:
  explicit TimedLogScopeWrapper(const char* title) : _title(title) {}

  void begin(Logger& logger, Level level) override;
  void end(Logger& logger, Level level) override;
};

class IndentedLogScopeWrapper : public LogScopeWrapper {
 private:
  const char* _title;

 public:
  explicit IndentedLogScopeWrapper(const char* title) : _title(title) {}

  void begin(Logger& logger, Level level) override;
  void end(Logger& logger, Level level) override;
};

class Logger {
 private:
  FILE* _ostream;
  Level _maxLevel;
  size_t _indentation{0};

  const size_t INDENTATION_SIZE = 2;

 public:
  Logger(FILE* location, Level level) : _ostream(location), _maxLevel(level) {}

  template <typename Action>
  void beginScopeProcedure(Level level, LogScopeWrapper& logScopeWrapper,
                           Action&& action) {
    action();
    logScopeWrapper.end(*this, level);
  }

  template <typename ReturnType, typename Action>
  ReturnType beginScopeFunction(Level level, LogScopeWrapper& logScopeWrapper,
                                Action&& action) {
    logScopeWrapper.begin(*this, level);
    assert(!std::is_void_v<ReturnType>);
    ReturnType value = action();
    logScopeWrapper.end(*this, level);
    return value;
  }

  template <typename Action>
  void timedProcedure(const char* title, Action&& action) {
    timedProcedure<Action>(Level::INFO, title, std::move(action));
  }

  template <typename ReturnType, typename Action>
  ReturnType timedFunction(const char* title, Action action) {
    return timedFunction<ReturnType, Action>(Level::INFO, title,
                                             std::move(action));
  }

  template <typename Action>
  void timedProcedure(Level level, const char* title, Action&& action) {
    TimedLogScopeWrapper wrapper(title);
    beginScopeProcedure<Action>(level, wrapper, std::move(action));
  }

  template <typename ReturnType, typename Action>
  ReturnType timedFunction(Level level, const char* title, Action&& action) {
    TimedLogScopeWrapper wrapper(title);
    return beginScopeFunction<ReturnType, Action>(level, wrapper,
                                                  std::move(action));
  }

  template <typename Action>
  void indentedProcedure(Level level, const char* title, Action&& action) {
    IndentedLogScopeWrapper wrapper(title);
    beginScopeProcedure<Action>(level, wrapper, std::move(action));
  }

  template <typename ReturnType, typename Action>
  ReturnType indentedFunction(Level level, const char* title, Action&& action) {
    IndentedLogScopeWrapper wrapper(title);
    return beginScopeFunction<ReturnType, Action>(level, wrapper,
                                                  std::move(action));
  }

  template <typename... T>
  void log(Level level, fmt::format_string<T...> format, T&&... args) {
    if (level <= _maxLevel) {
      fmt::print(_ostream, "{:<7} {: >{}}{}\n",
                 fmt::format("[{}]", levelString(level)), "", _indentation,
                 fmt::format(format, std::forward<T>(args)...));
    }
  }

  template <typename... T>
  void trace(fmt::format_string<T...> format, T&&... args) {
    log(Level::TRACE, format, std::forward<T>(args)...);
  }

  template <typename... T>
  void debug(fmt::format_string<T...> format, T&&... args) {
    log(Level::DEBUG, format, std::forward<T>(args)...);
  }

  template <typename... T>
  void info(fmt::format_string<T...> format, T&&... args) {
    log(Level::INFO, format, std::forward<T>(args)...);
  }

  template <typename... T>
  void warn(fmt::format_string<T...> format, T&&... args) {
    log(Level::WARN, format, std::forward<T>(args)...);
  }

  template <typename... T>
  void err(fmt::format_string<T...> format, T&&... args) {
    log(Level::ERR, format, std::forward<T>(args)...);
  }

  inline void increaseIndentation() noexcept {
    _indentation += INDENTATION_SIZE;
  }

  inline void decreaseIndentation() noexcept {
    _indentation -= INDENTATION_SIZE;
  }

 private:
  inline static const char* levelString(Level level) {
    switch (level) {
      case Level::ERR:
        return "ERR";
      case Level::WARN:
        return "WARN";
      case Level::INFO:
        return "INFO";
      case Level::DEBUG:
        return "DEBUG";
      case Level::TRACE:
        return "TRACE";
    }

    return "";
  }
};

}  // namespace atlantis::logging