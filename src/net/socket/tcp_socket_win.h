// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TCP_SOCKET_WIN_H_
#define NET_SOCKET_TCP_SOCKET_WIN_H_

#include <stdint.h>
#include <winsock2.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/non_thread_safe.h"
#include "base/win/object_watcher.h"
#include "net/base/address_family.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"

namespace net {

class AddressList;
class IOBuffer;
class IPEndPoint;

class NET_EXPORT TCPSocketWin : NON_EXPORTED_BASE(public base::NonThreadSafe),
                                public base::win::ObjectWatcher::Delegate  {
 public:
  TCPSocketWin();
  ~TCPSocketWin() override;

  int Open(AddressFamily family);

  // Both AdoptConnectedSocket and AdoptListenSocket take ownership of an
  // existing socket. AdoptConnectedSocket takes an already connected
  // socket. AdoptListenSocket takes a socket that is intended to accept
  // connection. In some sense, AdoptListenSocket is more similar to Open.
  int AdoptConnectedSocket(SOCKET socket, const IPEndPoint& peer_address);
  int AdoptListenSocket(SOCKET socket);

  int Bind(const IPEndPoint& address);

  int Listen(int backlog);
  int Accept(std::unique_ptr<TCPSocketWin>* socket,
             IPEndPoint* address,
             const CompletionCallback& callback);

  int Connect(const IPEndPoint& address, const CompletionCallback& callback);
  bool IsConnected() const;
  bool IsConnectedAndIdle() const;

  // Multiple outstanding requests are not supported.
  // Full duplex mode (reading and writing at the same time) is supported.
  int Read(IOBuffer* buf, int buf_len, const CompletionCallback& callback);
  int Write(IOBuffer* buf, int buf_len, const CompletionCallback& callback);

  int GetLocalAddress(IPEndPoint* address) const;
  int GetPeerAddress(IPEndPoint* address) const;

  // Sets various socket options.
  // The commonly used options for server listening sockets:
  // - SetExclusiveAddrUse().
  int SetDefaultOptionsForServer();
  // The commonly used options for client sockets and accepted sockets:
  // - SetNoDelay(true);
  // - SetKeepAlive(true, 45).
  void SetDefaultOptionsForClient();
  int SetExclusiveAddrUse();
  int SetReceiveBufferSize(int32_t size);
  int SetSendBufferSize(int32_t size);
  bool SetKeepAlive(bool enable, int delay);
  bool SetNoDelay(bool no_delay);

  void Close();

  // NOOP since TCP FastOpen is not implemented in Windows.
  void EnableTCPFastOpenIfSupported() {}

  bool IsValid() const { return socket_ != INVALID_SOCKET; }

  // Detachs from the current thread, to allow the socket to be transferred to
  // a new thread. Should only be called when the object is no longer used by
  // the old thread.
  void DetachFromThread();

 private:
  class Core;

  // base::ObjectWatcher::Delegate implementation.
  void OnObjectSignaled(HANDLE object) override;

  int AcceptInternal(std::unique_ptr<TCPSocketWin>* socket,
                     IPEndPoint* address);

  int DoConnect();
  void DoConnectComplete(int result);

  void LogConnectEnd(int net_error);

  int DoRead(IOBuffer* buf, int buf_len, const CompletionCallback& callback);
  void DidCompleteConnect();
  void DidCompleteWrite();
  void DidSignalRead();

  SOCKET socket_;

  HANDLE accept_event_;
  base::win::ObjectWatcher accept_watcher_;

  std::unique_ptr<TCPSocketWin>* accept_socket_;
  IPEndPoint* accept_address_;
  CompletionCallback accept_callback_;

  // The various states that the socket could be in.
  bool waiting_connect_;
  bool waiting_read_;
  bool waiting_write_;

  // The core of the socket that can live longer than the socket itself. We pass
  // resources to the Windows async IO functions and we have to make sure that
  // they are not destroyed while the OS still references them.
  scoped_refptr<Core> core_;

  // External callback; called when connect or read is complete.
  CompletionCallback read_callback_;

  // External callback; called when write is complete.
  CompletionCallback write_callback_;

  std::unique_ptr<IPEndPoint> peer_address_;
  // The OS error that a connect attempt last completed with.
  int connect_os_error_;

  DISALLOW_COPY_AND_ASSIGN(TCPSocketWin);
};

}  // namespace net

#endif  // NET_SOCKET_TCP_SOCKET_WIN_H_
