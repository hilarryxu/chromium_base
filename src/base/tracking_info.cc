// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/tracking_info.h"

#include <stddef.h>
// #include "base/tracked_objects.h"

namespace base {

TrackingInfo::TrackingInfo() {}

TrackingInfo::TrackingInfo(
    const tracked_objects::Location& posted_from,
    base::TimeTicks delayed_run_time)
    : delayed_run_time(delayed_run_time) {
}

TrackingInfo::~TrackingInfo() {}

}  // namespace base

