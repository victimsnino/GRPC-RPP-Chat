#include "chat_client.hpp"

#include <chat.grpc.pb.h>

#include <grpc++/create_channel.h>
#include <grpc++/support/sync_stream.h>

#include <rpp/rpp.hpp>
#include <rppgrpc/rppgrpc.hpp>
#include <common.hpp>

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

            bool IsBlocking() const override { return false; }
            const char* GetType() const override { return Consts::g_authenication_header; }

            grpc::Status GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name, const grpc::AuthContext& channel_auth_context, std::multimap<grpc::string, grpc::string>* metadata) override
            {
                metadata->insert(std::make_pair(Consts::g_authenication_header, ticket_));
                return grpc::Status::OK;
            }

        private:
            grpc::string ticket_;
        };
    }

    struct Handler::State
    {
        State(const std::string& token)
        : state{InitState(ctx, token)}
        {}

        ~State() 
        {
            state.messages.on_completed(); 
        }
        
        struct InnerState
        {
            rpp::dynamic_observable<ChatService::Proto::Event> events;
            rpp::dynamic_observer<std::string>                 messages;
        };

    private:
        static InnerState InitState(grpc::ClientContext& ctx, const std::string& token)
        {
            ctx.set_credentials(grpc::MetadataCredentialsFromPlugin(std::make_unique<Authenticator>(token)));

            rpp::subjects::serialized_publish_subject<std::string>    messages{};

            const auto reactor = new rppgrpc::client_bidi_reactor<ChatService::Proto::Event::Message, ChatService::Proto::Event>();
            messages.get_observable()
                | rpp::ops::map([](const std::string& txt) {
                      ChatService::Proto::Event::Message m{};
                      m.set_text(txt);
                      return m;
                  })
                | rpp::ops::subscribe(reactor->get_observer());
            ChatService::Proto::Server::NewStub(grpc::CreateChannel("localhost:50051", grpc::experimental::LocalCredentials(grpc_local_connect_type::LOCAL_TCP)))->async()->ChatStream(&ctx, reactor);
            reactor->init();

            return InnerState{.events = reactor->get_observable().as_dynamic(), .messages = messages.get_observer().as_dynamic()};
        }
    public:
        grpc::ClientContext ctx{};
        InnerState state;
    };

    Handler::Handler(const std::string& token)
        : m_state{std::make_shared<Handler::State>(token)}
    {

    }

    void Handler::SendMessage(const std::string& message) const
    {
        m_state->state.messages.on_next(message);
    }

    const rpp::dynamic_observable<ChatService::Proto::Event>& Handler::GetEvents() const
    {
        return m_state->state.events;
    }

}