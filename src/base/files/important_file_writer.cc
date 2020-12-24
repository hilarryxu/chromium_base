// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/important_file_writer.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <utility>

#include "build/build_config.h"
#include "base/logging.h"
#include "base/memory/owned_pointer.h"
#include "base/debug/alias.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/task_runner.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/message_loop/message_loop_proxy.h"

namespace base {

namespace {

const int kDefaultCommitIntervalMs = 10000;

// This enum is used to define the buckets for an enumerated UMA histogram.
// Hence,
//   (a) existing enumerated constants should never be deleted or reordered, and
//   (b) new constants should only be appended at the end of the enumeration.
enum TempFileFailure {
  FAILED_CREATING,
  FAILED_OPENING,
  FAILED_CLOSING,  // Unused.
  FAILED_WRITING,
  FAILED_RENAMING,
  FAILED_FLUSHING,
  TEMP_FILE_FAILURE_MAX
};

void LogFailure(const FilePath& path, TempFileFailure failure_code,
                const std::string& message) {
  // DPLOG(WARNING) << "temp file failure: " << path.value() << " : " << message;
}

// Helper function to call WriteFileAtomically() with a std::unique_ptr<std::string>.
bool WriteScopedStringToFileAtomically(const FilePath& path,
                                       owned_pointer<std::string> data) {
  return ImportantFileWriter::WriteFileAtomically(path, *data);
}

}  // namespace

// static
bool ImportantFileWriter::WriteFileAtomically(const FilePath& path,
                                              const std::string& data) {
  // Write the data to a temp file then rename to avoid data loss if we crash
  // while writing the file. Ensure that the temp file is on the same volume
  // as target file, so it can be moved in one step, and that the temp file
  // is securely created.
  FilePath tmp_file_path;
  if (!CreateTemporaryFileInDir(path.DirName(), &tmp_file_path)) {
    LogFailure(path, FAILED_CREATING, "could not create temporary file");
    return false;
  }

  File tmp_file(tmp_file_path, File::FLAG_OPEN | File::FLAG_WRITE);
  if (!tmp_file.IsValid()) {
    LogFailure(path, FAILED_OPENING, "could not open temporary file");
    return false;
  }

  // If this fails in the wild, something really bad is going on.
  const int data_length = checked_cast<int32_t>(data.length());
  int bytes_written = tmp_file.Write(0, data.data(), data_length);
  bool flush_success = tmp_file.Flush();
  tmp_file.Close();

  if (bytes_written < data_length) {
    LogFailure(path, FAILED_WRITING, "error writing, bytes_written=" +
               IntToString(bytes_written));
    DeleteFile(tmp_file_path, false);
    return false;
  }

  if (!flush_success) {
    LogFailure(path, FAILED_FLUSHING, "error flushing");
    DeleteFile(tmp_file_path, false);
    return false;
  }

  if (!ReplaceFile(tmp_file_path, path, nullptr)) {
    LogFailure(path, FAILED_RENAMING, "could not rename temporary file");
    DeleteFile(tmp_file_path, false);
    return false;
  }

  return true;
}

ImportantFileWriter::ImportantFileWriter(
    const FilePath& path,
    const std::shared_ptr<SequencedTaskRunner>& task_runner)
    : ImportantFileWriter(
        path,
        task_runner,
        TimeDelta::FromMilliseconds(kDefaultCommitIntervalMs)) {
}

ImportantFileWriter::ImportantFileWriter(
    const FilePath& path,
    const std::shared_ptr<SequencedTaskRunner>& task_runner,
    TimeDelta interval)
    : path_(path),
      task_runner_(task_runner),
      serializer_(nullptr),
      commit_interval_(interval) {
  DCHECK(CalledOnValidThread());
  DCHECK(task_runner_);
}

ImportantFileWriter::~ImportantFileWriter() {
  // We're usually a member variable of some other object, which also tends
  // to be our serializer. It may not be safe to call back to the parent object
  // being destructed.
  DCHECK(!HasPendingWrite());
}

bool ImportantFileWriter::HasPendingWrite() const {
  DCHECK(CalledOnValidThread());
  return timer_.IsRunning();
}

void ImportantFileWriter::WriteNow(std::string&& data) {
  DCHECK(CalledOnValidThread());
  if (!IsValueInRangeForNumericType<int32_t>(data.length())) {
    NOTREACHED();
    return;
  }

  if (HasPendingWrite())
    timer_.Stop();

  std::string* p_data = new std::string(std::move(data));
  owned_pointer<std::string> op_data(p_data);
  auto task = std::bind(WriteScopedStringToFileAtomically, path_, op_data);
  if (!PostWriteTask(task)) {
    // Posting the task to background message loop is not expected
    // to fail, but if it does, avoid losing data and just hit the disk
    // on the current thread.
    NOTREACHED();

    task();
  }
}

void ImportantFileWriter::ScheduleWrite(DataSerializer* serializer) {
  DCHECK(CalledOnValidThread());

  DCHECK(serializer);
  serializer_ = serializer;

  if (!timer_.IsRunning()) {
    timer_.Start(FROM_HERE,
                 commit_interval_,
                 std::bind(&ImportantFileWriter::DoScheduledWrite, this));
  }
}

void ImportantFileWriter::DoScheduledWrite() {
  DCHECK(serializer_);
  std::string data;
  if (serializer_->SerializeData(&data)) {
    WriteNow(std::move(data));
  } else {
    // DLOG(WARNING) << "failed to serialize data to be saved in "
    //               << path_.value();
  }
  serializer_ = nullptr;
}

void ImportantFileWriter::RegisterOnNextSuccessfulWriteCallback(
    const Closure& on_next_successful_write) {
  DCHECK(!on_next_successful_write_);
  on_next_successful_write_ = on_next_successful_write;
}

bool ImportantFileWriter::PostWriteTask(const std::function<bool()>& task) {
  // TODO(gab): This code could always use PostTaskAndReplyWithResult and let
  // ForwardSuccessfulWrite() no-op if |on_next_successful_write_| is null, but
  // PostTaskAndReply causes memory leaks in tests (crbug.com/371974) and
  // suppressing all of those is unrealistic hence we avoid most of them by
  // using PostTask() in the typical scenario below.
  if (on_next_successful_write_) {
    std::function<void(bool)> reply = base::Bind(&ImportantFileWriter::ForwardSuccessfulWrite, this, std::placeholders::_1);
    return task_runner_->PostTaskAndReply(
      FROM_HERE,
      task,
      reply
    );
  }
  return task_runner_->PostTask(FROM_HERE, task);
}

void ImportantFileWriter::ForwardSuccessfulWrite(bool result) {
  DCHECK(CalledOnValidThread());
  if (result && on_next_successful_write_) {
    on_next_successful_write_();
    on_next_successful_write_ = nullptr;
  }
}

}  // namespace base
