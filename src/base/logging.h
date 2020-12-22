// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <stdio.h>
#include <stdlib.h>
#include <ostream>
#include <sstream>

#include "base/base_export.h"
#include "base/compiler_specific.h"
#include "build/build_config.h"

#ifdef NDEBUG
#define ENABLE_DLOG 0
#define ENABLE_DCHECK 0
#define DCHECK_IS_ON() false
#else
#define ENABLE_DLOG 1
#define ENABLE_DCHECK 1
#define DCHECK_IS_ON() true
#endif

#define LOG_IS_ON(severity) true

#define LOG_DEBUG LogMessage(__FILE__, __LINE__)
#define LOG_INFO LogMessage(__FILE__, __LINE__)
#define LOG_WARNING LogMessage(__FILE__, __LINE__)
#define LOG_ERROR LogMessage(__FILE__, __LINE__)
#define LOG_FATAL LogMessageFatal(__FILE__, __LINE__)
#define LOG_QFATAL LOG_FATAL

#define LAZY_STREAM(stream, condition) \
  !(condition) ? (void)0 : LogMessageVoidify() & (stream)

#define LOG_STREAM(severity) LOG_##severity.stream()

#define LOG(severity) LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity))
#define LOG_IF(severity, condition) \
  LAZY_STREAM(LOG_STREAM(severity), (condition))
#define PLOG LOG

// Debug-only checking.
#define DCHECK(condition)                                        \
  LAZY_STREAM(LOG_STREAM(DEBUG), DCHECK_IS_ON() && !(condition)) \
      << "Check failed: " #condition ". "

#define DCHECK_EQ(val1, val2) DCHECK((val1) == (val2))
#define DCHECK_NE(val1, val2) DCHECK((val1) != (val2))
#define DCHECK_LE(val1, val2) DCHECK((val1) <= (val2))
#define DCHECK_LT(val1, val2) DCHECK((val1) < (val2))
#define DCHECK_GE(val1, val2) DCHECK((val1) >= (val2))
#define DCHECK_GT(val1, val2) DCHECK((val1) > (val2))

#define NOTREACHED() DCHECK(false)
#define DPCHECK DCHECK

#if defined(COMPILER_GCC)
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the NOTIMPLEMENTED message.
#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG

// Always-on checking
#define CHECK(x) \
  if (x) {       \
  } else         \
    LogMessageFatal(__FILE__, __LINE__).stream() << "Check failed: " #x
#define CHECK_LT(x, y) CHECK((x) < (y))
#define CHECK_GT(x, y) CHECK((x) > (y))
#define CHECK_LE(x, y) CHECK((x) <= (y))
#define CHECK_GE(x, y) CHECK((x) >= (y))
#define CHECK_EQ(x, y) CHECK((x) == (y))
#define CHECK_NE(x, y) CHECK((x) != (y))

// It seems that one of the Windows header files defines ERROR as 0.
#ifdef _WIN32
#define LOG_0 LOG_INFO
#define LOG_1 LOG_DEBUG
#endif

#ifdef NDEBUG
#define LOG_DFATAL LOG_ERROR
#else
#define LOG_DFATAL LOG_FATAL
#endif

#define DLOG(severity) \
  LAZY_STREAM(LOG_STREAM(severity), ENABLE_DLOG)
#define DLOG_IF(severity, condition) \
  LAZY_STREAM(LOG_STREAM(severity), ENABLE_DLOG && (condition))
#define DPLOG DLOG
#define DPLOG_IF DLOG_IF

#define DVLOG(x) \
  LAZY_STREAM(LOG_STREAM(INFO), ENABLE_DLOG)

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
 public:
  LogMessageVoidify() {}
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) {}
};

class LogMessage {
 public:
  LogMessage(const char* file, int line) : flushed_(false) {
    stream() << file << ":" << line << ": ";
  }
  void Flush() {
    stream() << "\n";
    std::string s = str_.str();
    size_t n = s.size();
    if (fwrite(s.data(), 1, n, stderr) < n) {
    }  // shut up gcc
    flushed_ = true;
  }
  ~LogMessage() {
    if (!flushed_) {
      Flush();
    }
  }
  std::ostream& stream() { return str_; }

 private:
  bool flushed_;
  std::ostringstream str_;

  LogMessage(const LogMessage&) = delete;
  LogMessage& operator=(const LogMessage&) = delete;
};

// Silence "destructor never returns" warning for ~LogMessageFatal().
// Since this is a header file, push and then pop to limit the scope.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4722)
#endif

class LogMessageFatal : public LogMessage {
 public:
  LogMessageFatal(const char* file, int line) : LogMessage(file, line) {}
  ~LogMessageFatal() {
    Flush();
    abort();
  }

 private:
  LogMessageFatal(const LogMessageFatal&) = delete;
  LogMessageFatal& operator=(const LogMessageFatal&) = delete;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  // BASE_LOGGING_H_
