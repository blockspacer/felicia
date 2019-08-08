#ifndef FELICIA_CORE_MASTER_RPC_GRPC_MASTER_CLIENT_H_
#define FELICIA_CORE_MASTER_RPC_GRPC_MASTER_CLIENT_H_

#include <utility>

#include "grpcpp/grpcpp.h"
#include "third_party/chromium/base/macros.h"
#include "third_party/chromium/base/threading/thread.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/error/status.h"
#include "felicia/core/master/master_client_interface.h"
#include "felicia/core/master/rpc/grpc_async_client_call.h"
#include "felicia/core/master/rpc/grpc_client_cq_tag.h"
#include "felicia/core/master/rpc/master_service.grpc.pb.h"

namespace felicia {

class EXPORT GrpcMasterClient : public MasterClientInterface {
 public:
  explicit GrpcMasterClient(std::shared_ptr<::grpc::Channel> channel);
  ~GrpcMasterClient();

  Status Start() override;
  Status Stop() override;

#define CLIENT_METHOD(method)                                             \
  void method##Async(const method##Request* request,                      \
                     method##Response* response, StatusOnceCallback done) \
      override

  CLIENT_METHOD(RegisterClient);
  CLIENT_METHOD(ListClients);
  CLIENT_METHOD(RegisterNode);
  CLIENT_METHOD(UnregisterNode);
  CLIENT_METHOD(ListNodes);
  CLIENT_METHOD(PublishTopic);
  CLIENT_METHOD(UnpublishTopic);
  CLIENT_METHOD(SubscribeTopic);
  CLIENT_METHOD(UnsubscribeTopic);
  CLIENT_METHOD(ListTopics);

#undef CLIENT_METHOD

  void HandleRpcsLoop();

 private:
  std::unique_ptr<grpc::MasterService::Stub> stub_;
  ::grpc::CompletionQueue cq_;
  std::vector<std::unique_ptr<base::Thread>> threads_;

  DISALLOW_COPY_AND_ASSIGN(GrpcMasterClient);
};

}  // namespace felicia

#endif  // FELICIA_CORE_MASTER_RPC_GRPC_MASTER_CLIENT_H_