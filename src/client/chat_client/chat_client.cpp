#include "chat_client.hpp"

#include <chat.grpc.pb.h>
#include <grpc++/create_channel.h>

namespace ChatClient
{
    namespace
    {
        class Authenticator final : public grpc::MetadataCredentialsPlugin
        {
        public:
            Authenticator(const grpc::string& ticket)
                : ticket_(ticket)
            {
            }

            grpc::Status GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name, const grpc::AuthContext& channel_auth_context, std::multimap<grpc::string, grpc::string>* metadata) override
            {
                metadata->insert(std::make_pair("Authorization", ticket_));
                return grpc::Status::OK;
            }

        private:
            grpc::string ticket_;
        };
    }

    Handler::Handler(std::string token)
        : m_stream{ChatService::Proto::Server::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()))->PrepareAsyncChatStream(::grpc::ClientContext *context)}
    {
        
    }

}