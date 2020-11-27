// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CALLBACK_FORWARD_H_
#define BASE_CALLBACK_FORWARD_H_

#include <functional>

namespace base {

using Closure = std::function<void(void)>;

}  // namespace base

#endif  // BASE_CALLBACK_FORWARD_H
