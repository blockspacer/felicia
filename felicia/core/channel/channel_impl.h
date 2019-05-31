#ifndef FELICIA_CORE_CHANNEL_CHANNEL_IMPL_H_
#define FELICIA_CORE_CHANNEL_CHANNEL_IMPL_H_

#include "third_party/chromium/net/base/io_buffer.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/error/status.h"

namespace felicia {

class Socket;

class ChannelImpl {
 public:
  virtual ~ChannelImpl();

  virtual bool IsSocket() const;

  Socket* ToSocket();

  virtual void Write(scoped_refptr<::net::IOBuffer> buffer, int size,
                     StatusOnceCallback callback) = 0;
  virtual void Read(scoped_refptr<::net::GrowableIOBuffer> buffer, int size,
                    StatusOnceCallback callback) = 0;
};

}  // namespace felicia

#endif  // FELICIA_CORE_CHANNEL_CHANNEL_IMPL_H_