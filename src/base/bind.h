// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_BIND_H_
#define BASE_BIND_H_

#include <functional>

namespace base {

// global function
template<class F, class... Args, class = typename std::enable_if<!std::is_member_function_pointer<F>::value>::type>
auto Bind(F && f, Args && ... args) -> decltype(std::bind(f, args...))
{
  return std::bind(f, args...);
}

}  // namespace base

#endif  // BASE_BIND_H_
