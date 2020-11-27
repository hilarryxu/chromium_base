// Copyright (c) 2011, NetEase Inc. All rights reserved.
//
// Author: rl
// Date: 2011/6/29
//
// Enhanced by: wrt(guangguang)
//
// Thread with framework(message loop)

#include "base/threading/framework_thread.h"

#include "base/logging.h"
#include "base/lazy_instance.h"
#include "base/callback.h"

namespace base {

// We use this thread-local variable to record whether or not a thread exited
// because its Stop method was called.  This allows us to catch cases where
// MessageLoop::Quit() is called directly, which is unexpected when using a
// Thread to setup and run a MessageLoop.
base::LazyInstance<base::ThreadLocalPointer<FrameworkThreadTlsData> >
    lazy_tls_data;

FrameworkThread::FrameworkThread(const char* name)
    : started_(false),
      stopping_(false),
      loop_type_(MessageLoop::kDefaultMessageLoop),
      message_loop_(nullptr),
      event_(false, false),
      name_(name),
      thread_(0),
      id_(kInvalidThreadId) {}

FrameworkThread::~FrameworkThread() {
  Stop();
}

bool FrameworkThread::Start() {
  return StartWithLoop(MessageLoop::kDefaultMessageLoop);
}

#if defined(OS_WIN)
bool FrameworkThread::StartWithLoop(const MessageLoop::Type type,
                                    Dispatcher* dispatcher)
#else
bool FrameworkThread::StartWithLoop(const MessageLoop::Type type)
#endif
{
  if (message_loop_ != nullptr)
    return false;

  loop_type_ = type;
#if defined(OS_WIN)
  dispatcher_ = dispatcher;
#endif

  if (!Create())
    return false;

  // Wait for the thread to start and initialize message_loop_
  event_.Wait();

  started_ = true;
  return true;
}

#if defined(OS_WIN)
bool FrameworkThread::StartWithLoop(CustomMessageLoopFactory* factory,
                                    Dispatcher* dispatcher)
#else
bool FrameworkThread::StartWithLoop(CustomMessageLoopFactory* factory)
#endif
{
  if (factory == nullptr)
    return false;

  factory_.reset(factory);
  loop_type_ = MessageLoop::kCustomMessageLoop;
#if defined(OS_WIN)
  dispatcher_ = dispatcher;
#endif

  if (!Create())
    return false;

  // Wait for the thread to start and initialize message_loop_
  event_.Wait();

  started_ = true;

  return true;
}

bool FrameworkThread::Create() {
  id_ = kInvalidThreadId;

  SetThreadWasQuitProperly(false);

  // Hold the thread_lock_ while starting a new thread, so that we can make sure
  // that thread_ is populated before the newly created thread accesses it.
  {
    AutoLock lock(thread_lock_);
    if (!PlatformThread::Create(0, this, &thread_)) {
      DLOG(ERROR) << "failed to create thread";
      return false;
    }
  }

  return true;
}

#if defined(OS_WIN)
void FrameworkThread::RunOnCurrentThreadWithLoop(const MessageLoop::Type type,
                                                 Dispatcher* dispatcher)
#else
void FrameworkThread::RunOnCurrentThreadWithLoop(const MessageLoop::Type type)
#endif
{
  loop_type_ = type;
  started_ = true;
#if defined(OS_WIN)
  dispatcher_ = dispatcher;
#endif
  ThreadMain();
}

void FrameworkThread::Stop() {
  AutoLock lock(thread_lock_);
  if (thread_.is_null())
    return;

  StopSoon();

  // Wait for the thread to exit.
  //
  // TODO(darin): Unfortunately, we need to keep message_loop_ around until
  // the thread exits.  Some consumers are abusing the API.  Make them stop.
  //
  PlatformThread::Join(thread_);
  thread_ = base::PlatformThreadHandle();

  // The thread should nullify message_loop_ on exit.
  DCHECK(!message_loop_);

  started_ = false;
  stopping_ = false;
}

void FrameworkThread::StopSoon() {
  if (stopping_ || !message_loop_)
    return;

  stopping_ = true;
  message_loop_->PostNonNestableTask(
      base::Bind(&FrameworkThread::DoStopSoon, this));
}

void FrameworkThread::DoStopSoon() {
  MessageLoop::current()->Quit();
  SetThreadWasQuitProperly(true);
}

bool FrameworkThread::IsRunning() const {
  AutoLock lock(thread_lock_);
  return !thread_.is_null();
}

void FrameworkThread::ThreadMain() {
  id_ = PlatformThread::CurrentId();
  PlatformThread::SetName(name_.c_str());

  InitTlsData(this);
  SetThreadWasQuitProperly(false);
  {
    // The message loop for this thread.
    MessageLoop* message_loop;
    if (loop_type_ == MessageLoop::kCustomMessageLoop)
      message_loop = factory_->CreateMessageLoop();
    else {
      // if (loop_type_ == MessageLoop::kIOMessageLoop)
      // 	message_loop = new IOMessageLoop;
#if defined(OS_WIN)
      if (loop_type_ == MessageLoop::kUIMessageLoop)
        message_loop = new UIMessageLoop;
#endif
      else
        message_loop = new MessageLoop;
    }
    message_loop_ = message_loop;

    // Let the thread do extra initialization.
    // Let's do this before signaling we are started.
    Init();

    event_.Signal();

#if defined(OS_WIN)
    message_loop_->RunWithDispatcher(dispatcher_);
#else
    message_loop_->Run();
#endif  // OS_WIN

    // Let the thread do extra cleanup.
    Cleanup();

    DCHECK(GetThreadWasQuitProperly());

    // We can't receive messages anymore.
    if (loop_type_ != MessageLoop::kCustomMessageLoop)
      delete message_loop_;
    else {
      delete message_loop_;
      factory_.reset();
    }
    message_loop_ = nullptr;
  }
  id_ = kInvalidThreadId;
  {
    FrameworkThreadTlsData* tls = GetTlsData();
    if (tls != nullptr) {
    }
    DCHECK(tls->managed == 0);
    // you must call
    // ThreadManager::UnregisterThread before come there
  }
  FreeTlsData();
}

void FrameworkThread::InitTlsData(FrameworkThread* self) {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls == nullptr);
  if (tls != nullptr)
    return;
  tls = new FrameworkThreadTlsData;
  tls->self = self;
  tls->managed = 0;
  tls->managed_thread_id = -1;
  tls->quit_properly = false;
  tls->custom_data = nullptr;
  lazy_tls_data.Pointer()->Set(tls);
}

void FrameworkThread::FreeTlsData() {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return;
  lazy_tls_data.Pointer()->Set(nullptr);
  delete tls;
}

FrameworkThreadTlsData* FrameworkThread::GetTlsData() {
  return lazy_tls_data.Pointer()->Get();
}

bool FrameworkThread::GetThreadWasQuitProperly() {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return false;
  return tls->quit_properly;
}

void FrameworkThread::SetThreadWasQuitProperly(bool flag) {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return;
  tls->quit_properly = flag;
}

FrameworkThread* FrameworkThread::current() {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return nullptr;
  return tls->self;
}

int FrameworkThread::GetManagedThreadId() {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return -1;
  return tls->managed_thread_id;
}

void* FrameworkThread::GetCustomTlsData() {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return nullptr;
  return tls->custom_data;
}

void FrameworkThread::SetCustomTlsData(void* data) {
  FrameworkThreadTlsData* tls = GetTlsData();
  DCHECK(tls != nullptr);
  if (tls == nullptr)
    return;
  tls->custom_data = data;
}

}  // namespace base
