// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/worker_pool.h"

#include "base/compiler_specific.h"
#include "base/debug/leak_annotations.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/task/task_runner.h"
// NOTE(xcc): 用到 PostTaskAndReply 的地方需要包含该头文件
#include "base/task/single_thread_task_runner.h"

namespace base {

namespace {

// WorkerPoolTaskRunner ---------------------------------------------
// A TaskRunner which posts tasks to a WorkerPool with a
// fixed ShutdownBehavior.
//
// Note that this class is RefCountedThreadSafe (inherited from TaskRunner).
class WorkerPoolTaskRunner : public TaskRunner {
 public:
  explicit WorkerPoolTaskRunner(bool tasks_are_slow);
  ~WorkerPoolTaskRunner() override;

  // TaskRunner implementation
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const Closure& task,
                       TimeDelta delay) override;
  bool RunsTasksOnCurrentThread() const override;

 private:

  // Helper function for posting a delayed task. Asserts that the delay is
  // zero because non-zero delays are not supported.
  bool PostDelayedTaskAssertZeroDelay(
      const tracked_objects::Location& from_here,
      const Closure& task,
      base::TimeDelta delay);

  const bool tasks_are_slow_;

  DISALLOW_COPY_AND_ASSIGN(WorkerPoolTaskRunner);
};

WorkerPoolTaskRunner::WorkerPoolTaskRunner(bool tasks_are_slow)
    : tasks_are_slow_(tasks_are_slow) {
}

WorkerPoolTaskRunner::~WorkerPoolTaskRunner() {
}

bool WorkerPoolTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const Closure& task,
    TimeDelta delay) {
  return PostDelayedTaskAssertZeroDelay(from_here, task, delay);
}

bool WorkerPoolTaskRunner::RunsTasksOnCurrentThread() const {
  return WorkerPool::RunsTasksOnCurrentThread();
}

bool WorkerPoolTaskRunner::PostDelayedTaskAssertZeroDelay(
    const tracked_objects::Location& from_here,
    const Closure& task,
    base::TimeDelta delay) {
  DCHECK_EQ(delay.InMillisecondsRoundedUp(), 0)
      << "WorkerPoolTaskRunner does not support non-zero delays";
  return WorkerPool::PostTask(from_here, task, tasks_are_slow_);
}

struct TaskRunnerHolder {
  TaskRunnerHolder() {
    taskrunners_[0] = std::make_shared<WorkerPoolTaskRunner>(new WorkerPoolTaskRunner(false));
    taskrunners_[1] = std::make_shared<WorkerPoolTaskRunner>(new WorkerPoolTaskRunner(true));
  }
  std::shared_ptr<TaskRunner> taskrunners_[2];
};

base::LazyInstance<TaskRunnerHolder>::Leaky
    g_taskrunners = LAZY_INSTANCE_INITIALIZER;

}  // namespace

bool WorkerPool::PostTaskAndReply(const tracked_objects::Location& from_here,
                                  const Closure& task,
                                  const Closure& reply,
                                  bool task_is_slow) {
  // Do not report PostTaskAndReplyRelay leaks in tests. There's nothing we can
  // do about them because WorkerPool doesn't have a flushing API.
  // http://crbug.com/248513
  // http://crbug.com/290897
  // Note: this annotation does not cover tasks posted through a TaskRunner.
  ANNOTATE_SCOPED_MEMORY_LEAK;
  return WorkerPool::GetTaskRunner(task_is_slow)->PostTaskAndReply(
      from_here, task, reply);
}

// static
std::shared_ptr<TaskRunner>
WorkerPool::GetTaskRunner(bool tasks_are_slow) {
  return g_taskrunners.Get().taskrunners_[tasks_are_slow];
}

}  // namespace base
