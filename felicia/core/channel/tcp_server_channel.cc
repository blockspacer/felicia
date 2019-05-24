#include "felicia/core/channel/tcp_server_channel.h"

#include <utility>

#include "third_party/chromium/base/bind.h"
#include "third_party/chromium/base/strings/strcat.h"
#include "third_party/chromium/base/threading/thread_task_runner_handle.h"
#include "third_party/chromium/net/base/net_errors.h"

#include "felicia/core/lib/error/errors.h"
#include "felicia/core/lib/net/net_util.h"

namespace felicia {

TCPServerChannel::TCPServerChannel() = default;
TCPServerChannel::~TCPServerChannel() = default;

const std::vector<std::unique_ptr<::net::TCPSocket>>&
TCPServerChannel::accepted_sockets() const {
  return accepted_sockets_;
}

bool TCPServerChannel::IsServer() const { return true; }

bool TCPServerChannel::IsConnected() const {
  for (auto& accepted_socket : accepted_sockets_) {
    if (accepted_socket->IsConnected()) return true;
  }
  return false;
}

StatusOr<ChannelSource> TCPServerChannel::Listen() {
  auto server_socket = std::make_unique<::net::TCPSocket>(nullptr);

  int rv = server_socket->Open(::net::ADDRESS_FAMILY_IPV4);
  if (rv != ::net::OK) {
    return errors::NetworkError(::net::ErrorToString(rv));
  }

  uint16_t port = net::PickRandomPort(true);
  ::net::IPAddress address(0, 0, 0, 0);
  ::net::IPEndPoint server_endpoint(address, port);
  rv = server_socket->Bind(server_endpoint);
  if (rv != ::net::OK) {
    return errors::NetworkError(::net::ErrorToString(rv));
  }

  rv = server_socket->SetDefaultOptionsForServer();
  if (rv != ::net::OK) {
    return errors::NetworkError(::net::ErrorToString(rv));
  }

  rv = server_socket->Listen(5);
  if (rv != ::net::OK) {
    return errors::NetworkError(::net::ErrorToString(rv));
  }

  socket_ = std::move(server_socket);

  return ToChannelSource(
      ::net::IPEndPoint(net::HostIPAddress(net::HOST_IP_ONLY_ALLOW_IPV4), port),
      ChannelDef::TCP);
}

void TCPServerChannel::DoAcceptLoop(AcceptCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(accept_callback_.is_null() && accept_once_callback_.is_null());
  accept_callback_ = callback;
  DoAcceptLoop();
}

void TCPServerChannel::AcceptOnce(AcceptOnceCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(accept_callback_.is_null() && accept_once_callback_.is_null());
  accept_once_callback_ = std::move(callback);
  DoAccept();
}

void TCPServerChannel::Write(char* buffer, int size,
                             StatusOnceCallback callback) {
  DCHECK_EQ(0, to_write_count_);
  DCHECK_EQ(0, written_count_);
  DCHECK(write_callback_.is_null());
  DCHECK(!callback.is_null());
  DCHECK(size > 0);

  EraseClosedSockets();

  if (accepted_sockets_.size() == 0) {
    std::move(callback).Run(errors::NetworkError(
        ::net::ErrorToString(::net::ERR_SOCKET_NOT_CONNECTED)));
    return;
  }

  to_write_count_ = accepted_sockets_.size();
  write_callback_ = std::move(callback);
  auto it = accepted_sockets_.begin();
  while (it != accepted_sockets_.end()) {
    int to_write = size;
    int written = 0;
    while (to_write > 0) {
      scoped_refptr<::net::IOBufferWithSize> write_buffer =
          base::MakeRefCounted<::net::IOBufferWithSize>(to_write);
      memcpy(write_buffer->data(), buffer + written, to_write);
      int rv =
          (*it)->Write(write_buffer.get(), write_buffer->size(),
                       ::base::BindOnce(&TCPServerChannel::OnWrite,
                                        ::base::Unretained(this), (*it).get()),
                       ::net::DefineNetworkTrafficAnnotation(
                           "tcp_server_channel", "Send Message"));
      if (rv == ::net::ERR_IO_PENDING) break;

      if (rv >= 0) {
        to_write -= rv;
        written += rv;
      }

      if (to_write == 0 || rv <= 0) {
        OnWrite((*it).get(), rv);
        break;
      }
    }

    it++;
  }
}

void TCPServerChannel::Read(char* buffer, int size,
                            StatusOnceCallback callback) {
  DCHECK(read_callback_.is_null());
  DCHECK(!callback.is_null());
  DCHECK(size > 0);

  EraseClosedSockets();

  if (accepted_sockets_.size() == 0) {
    std::move(callback).Run(errors::NetworkError(
        ::net::ErrorToString(::net::ERR_SOCKET_NOT_CONNECTED)));
    return;
  }

  read_callback_ = std::move(callback);
  auto it = accepted_sockets_.rbegin();

  int to_read = size;
  int read = 0;
  while (to_read > 0) {
    scoped_refptr<::net::IOBufferWithSize> read_buffer =
        base::MakeRefCounted<::net::IOBufferWithSize>(to_read);
    int rv =
        (*it)->Read(read_buffer.get(), read_buffer->size(),
                    ::base::BindOnce(&TCPServerChannel::OnReadAsync,
                                     ::base::Unretained(this), buffer + read,
                                     read_buffer, (*it).get()));

    if (rv == ::net::ERR_IO_PENDING) break;

    if (rv > 0) {
      memcpy(buffer + read, read_buffer->data(), rv);
      to_read -= rv;
      read += rv;
    }

    if (to_read == 0 || rv <= 0) {
      OnRead((*it).get(), rv);
    }
  }
}

int TCPServerChannel::DoAccept() {
  int result = socket_->Accept(
      &accepted_socket_, &accepted_endpoint_,
      ::base::BindOnce(&TCPServerChannel::OnAccept, ::base::Unretained(this)));
  if (result != ::net::ERR_IO_PENDING) HandleAccpetResult(result);
  return result;
}

void TCPServerChannel::DoAcceptLoop() {
  int result = ::net::OK;
  while (result == ::net::OK) {
    result = DoAccept();
  }
}

void TCPServerChannel::HandleAccpetResult(int result) {
  DCHECK_NE(result, ::net::ERR_IO_PENDING);

  if (result < 0) {
    LOG(ERROR) << "Error when accepting a connection: "
               << ::net::ErrorToString(result);
    return;
  }

  accepted_sockets_.push_back(std::move(accepted_socket_));
  if (accept_callback_)
    accept_callback_.Run(Status::OK());
  else
    std::move(accept_once_callback_).Run(Status::OK());
}

void TCPServerChannel::OnAccept(int result) {
  HandleAccpetResult(result);
  if (result == ::net::OK) {
    if (accept_callback_) DoAcceptLoop();
  }
}

void TCPServerChannel::OnReadAsync(
    char* buffer, scoped_refptr<::net::IOBufferWithSize> read_buffer,
    ::net::TCPSocket* socket, int result) {
  if (result > 0) memcpy(buffer, read_buffer->data(), result);
  OnRead(socket, result);
}

void TCPServerChannel::OnRead(::net::TCPSocket* socket, int result) {
  if (result == 0) {
    result = ::net::ERR_CONNECTION_CLOSED;
    socket->Close();
    has_closed_sockets_ = true;
  }
  CallbackWithStatus(std::move(read_callback_), result);
}

void TCPServerChannel::OnWrite(::net::TCPSocket* socket, int result) {
  if (result == ::net::ERR_CONNECTION_RESET) {
    socket->Close();
    has_closed_sockets_ = true;
  }

  written_count_++;
  if (result < 0) {
    LOG(ERROR) << "TCPServerChannel::OnWrite: " << ::net::ErrorToString(result);
    write_result_ = result;
  }
  if (to_write_count_ == written_count_) {
    to_write_count_ = 0;
    written_count_ = 0;
    int write_result = write_result_;
    write_result_ = 0;
    CallbackWithStatus(std::move(write_callback_), write_result);
  }
}

void TCPServerChannel::EraseClosedSockets() {
  if (has_closed_sockets_) {
    auto it = accepted_sockets_.begin();
    while (it != accepted_sockets_.end()) {
      if (!(*it)->IsConnected()) {
        it = accepted_sockets_.erase(it);
        continue;
      }
      it++;
    }
    has_closed_sockets_ = false;
  }
}

}  // namespace felicia