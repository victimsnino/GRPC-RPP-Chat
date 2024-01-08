#include "chat_client.hpp"

#include <chat.grpc.pb.h>

#include <grpc++/create_channel.h>
#include <grpc++/support/sync_stream.h>

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

        struct Reactor final : public grpc::ClientBidiReactor< ::ChatService::Proto::Event_Message, ::ChatService::Proto::Event>
        {
            void OnDone(const grpc::Status& /*s*/) override {}
            void OnReadDone(bool /*ok*/) override {}
            void OnWriteDone(bool /*ok*/) override {}
            void OnWritesDoneDone(bool /*ok*/) override {}

            // rpp::subjects::publish_subject<ChatService::Proto::Event> subject{};
            // Proto::Event::Message message{};
        };
    }

    Handler::Handler(std::string token)
    {
        ChatService::Proto::Server::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()))->async()->ChatStream(&m_context, new Reactor());
    }

    Handler::~Handler() noexcept = default;

}