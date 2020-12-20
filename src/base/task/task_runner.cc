// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task/task_runner.h"

#include "base/compiler_specific.h"
#include "base/logging.h"

namespace base {

bool TaskRunner::PostTask(const Closure& task) {
  return PostDelayedTask(task, base::TimeDelta());
}

TaskRunner::TaskRunner() {}

TaskRunner::~TaskRunner() {}

void TaskRunner::OnDestruct() const {
  delete this;
}

}  // namespace base
