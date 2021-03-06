// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/channel/socket/stream_socket_broadcaster.h"

#include "felicia/core/channel/socket/socket.h"
#include "felicia/core/lib/error/errors.h"

namespace felicia {

StreamSocketBroadcaster::StreamSocketBroadcaster(
    std::vector<std::unique_ptr<StreamSocket>>* sockets)
    : sockets_(sockets) {}

StreamSocketBroadcaster::~StreamSocketBroadcaster() = default;

void StreamSocketBroadcaster::Broadcast(scoped_refptr<net::IOBuffer> buffer,
                                        int size, StatusOnceCallback callback) {
  DCHECK_EQ(0, to_write_count_);
  DCHECK_EQ(0, written_count_);
  DCHECK(callback_.is_null());
  DCHECK(!callback.is_null());
  DCHECK(size > 0);

  EraseClosedSockets();

  if (sockets_->size() == 0) {
    std::move(callback).Run(errors::NetworkError(
        net::ErrorToString(net::ERR_SOCKET_NOT_CONNECTED)));
    return;
  }

  to_write_count_ = sockets_->size();
  callback_ = std::move(callback);
  auto it = sockets_->begin();
  while (it != sockets_->end()) {
    scoped_refptr<net::DrainableIOBuffer> write_buffer =
        base::MakeRefCounted<net::DrainableIOBuffer>(buffer,
                                                     static_cast<size_t>(size));
    while (write_buffer->BytesRemaining() > 0) {
      int rv =
          (*it)->Write(write_buffer.get(), write_buffer->BytesRemaining(),
                       base::BindOnce(&StreamSocketBroadcaster::OnWrite,
                                      base::Unretained(this), (*it).get()));

      if (rv == net::ERR_IO_PENDING) break;

      if (rv >= 0) {
        write_buffer->DidConsume(rv);
      }

      if (write_buffer->BytesRemaining() == 0 || rv <= 0) {
        OnWrite((*it).get(), rv);
        break;
      }
    }

    it++;
  }
}

void StreamSocketBroadcaster::OnWrite(StreamSocket* socket, int result) {
  if (result == net::ERR_CONNECTION_RESET ||
      (socket->IsStreamSocket() && result == net::ERR_FAILED)) {
    socket->Close();
    has_closed_sockets_ = true;
  }

  written_count_++;
  if (result < 0) {
    LOG(ERROR) << "StreamSocketBroadcaster::OnWrite: "
               << net::ErrorToString(result);
    write_result_ = result;
  }
  if (to_write_count_ == written_count_) {
    to_write_count_ = 0;
    written_count_ = 0;
    int write_result = write_result_;
    write_result_ = 0;
    Socket::CallbackWithStatus(std::move(callback_), write_result);
  }
}

void StreamSocketBroadcaster::EraseClosedSockets() {
  if (has_closed_sockets_) {
    auto it = sockets_->begin();
    while (it != sockets_->end()) {
      if (!(*it)->IsConnected()) {
        it = sockets_->erase(it);
        continue;
      }
      it++;
    }
    has_closed_sockets_ = false;
  }
}

}  // namespace felicia