// This file trys to implement a cross flatform message loop proxy,
// the mechanism of which is from the Google Chrome project.

#ifndef BASE_MESSAGE_LOOP_PROXY_H_
#define BASE_MESSAGE_LOOP_PROXY_H_
#pragma once

#include "base/base_export.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/lock.h"
#include "base/callback.h"
#include "base/task/single_thread_task_runner.h"

namespace base {

class MessageLoop;
class MessageLoopProxy;

struct BASE_EXPORT MessageLoopProxyTraits {
  static void Destruct(const MessageLoopProxy* proxy);
};

// A stock implementation of MessageLoopProxy that is created and managed by a
// MessageLoop. For now a MessageLoopProxy can only be created as part of a
// MessageLoop.
class BASE_EXPORT MessageLoopProxy : public SingleThreadTaskRunner, public base::SupportWeakCallback {
 public:
  static std::shared_ptr<MessageLoopProxy> current();

  // MessageLoopProxy implementation
  bool PostDelayedTask(const Closure& task, TimeDelta delay) override;
  bool PostNonNestableDelayedTask(const Closure& task, TimeDelta delay) override;

  bool RunsTasksOnCurrentThread() const override;
  ~MessageLoopProxy() override;

 private:
  // Allow the messageLoop to create a MessageLoopProxy.
  friend class MessageLoop;
  friend struct MessageLoopProxyTraits;

  MessageLoopProxy();

  // Called directly by MessageLoop::~MessageLoop.
  void WillDestroyCurrentMessageLoop();

  // Called when the reference decreased to 0
  void OnDestruct() const override;

  bool PostTaskHelper(const Closure& task, TimeDelta delay, bool nestable);

  void DeleteSelf() const;

  // The lock that protects access to target_message_loop_.
  mutable Lock message_loop_lock_;
  MessageLoop* target_message_loop_;
};

}  // namespace base

#endif  // BASE_MESSAGE_LOOP_PROXY_H_
