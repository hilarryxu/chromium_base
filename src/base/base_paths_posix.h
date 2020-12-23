// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_BASE_PATHS_POSIX_H_
#define BASE_BASE_PATHS_POSIX_H_

#include "base/base_export.h"

// This file declares windows-specific path keys for the base module.
// These can be used with the PathService to access various special
// directories and files.

namespace base {

enum {
  PATH_POSIX_START = 400,

  DIR_CACHE,    // Directory where to put cache data.  Note this is
                // *not* where the browser cache lives, but the
                // browser cache can be a subdirectory.
                // This is $XDG_CACHE_HOME on Linux and
                // ~/Library/Caches on Mac.
  PATH_POSIX_END
};

// /proc/self/exe refers to the current executable.
BASE_EXPORT extern const char kProcSelfExe[];

}  // namespace base

#endif  // BASE_BASE_PATHS_POSIX_H_
