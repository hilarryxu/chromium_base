// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task/sequenced_task_runner.h"

namespace base {

bool SequencedTaskRunner::PostNonNestableTask(const tracked_objects::Location& from_here,
                                              const Closure& task) {
  return PostNonNestableDelayedTask(from_here, task, base::TimeDelta());
}

}  // namespace base
