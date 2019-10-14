#include "felicia/core/channel/socket/ssl_server_context.h"
#include "felicia/core/felicia_init.h"
#include "felicia/core/master/master_proxy.h"
#include "felicia/examples/learn/topic/common/cc/topic_create_flag.h"
#include "felicia/examples/learn/topic/protobuf/cc/protobuf_publishing_node.h"
#include "felicia/examples/learn/topic/protobuf/cc/protobuf_subscribing_node.h"

namespace felicia {

int RealMain(int argc, char* argv[]) {
  FeliciaInit();

  TopicCreateFlag delegate;
  FlagParser parser;
  parser.set_program_name("protobuf_node_creator");
  if (!parser.Parse(argc, argv, &delegate)) {
    return 1;
  }

  MasterProxy& master_proxy = MasterProxy::GetInstance();
  Status s = master_proxy.Start();
  if (!s.ok()) {
    std::cerr << kRedError << s << std::endl;
    return 1;
  }

  NodeInfo node_info;
  node_info.set_name(delegate.name_flag()->value());

  std::unique_ptr<SSLServerContext> ssl_server_context;
  if (delegate.is_publishing_node_flag()->value()) {
    if (delegate.use_ssl_flag()->value()) {
      base::FilePath cert_file_path = base::FilePath(
          FILE_PATH_LITERAL("./felicia/examples/cert/server.crt"));
      base::FilePath private_key_file_path = base::FilePath(
          FILE_PATH_LITERAL("./felicia/examples/cert/server.key"));
      ssl_server_context = SSLServerContext::NewSSLServerContext(
          cert_file_path, private_key_file_path);
    }
    master_proxy.RequestRegisterNode<ProtobufPublishingNode>(
        node_info, delegate, ssl_server_context.get());
  } else {
    master_proxy.RequestRegisterNode<ProtobufSubscribingNode>(node_info,
                                                              delegate);
  }

  master_proxy.Run();

  return 0;
}

}  // namespace felicia

int main(int argc, char* argv[]) { return felicia::RealMain(argc, argv); }