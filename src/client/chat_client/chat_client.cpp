#include "chat_client.hpp"

#include <chat.grpc.pb.h>

#include <grpc++/create_channel.h>
#include <grpc++/support/sync_stream.h>

#include <rpp/rpp.hpp>
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

            grpc::Status GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name, const grpc::AuthContext& channel_auth_context, std::multimap<grpc::string, grpc::string>* metadata) override
            {
                metadata->insert(std::make_pair("Authorization", ticket_));
                return grpc::Status::OK;
            }

        private:
            grpc::string ticket_;
        };

        class Reactor final : public grpc::ClientBidiReactor< ::ChatService::Proto::Event_Message, ::ChatService::Proto::Event>
        {
            public:
                Reactor(const rpp::dynamic_observable<ChatService::Proto::Event::Message>& messages, const rpp::dynamic_observer<ChatService::Proto::Event>& events)
                    : m_observer{events}
                    , m_disposable{messages.subscribe_with_disposable([this](const ChatService::Proto::Event::Message& message) {
                        std::lock_guard lock{m_write_mutex};
                        m_write.push_back(message);
                        if (m_write.size() == 1)
                            StartWrite(&m_write.front());
                    },
                    [this]()
                    {
                        StartWritesDone();
                    })}
                {
                    
                }

                void Init()
                {
                    StartRead(&m_read);
                    StartCall();
                }

            private:
                void OnReadDone(bool ok) override 
                {
                    ENSURE(ok);

                    m_observer.on_next(m_read);
                    StartRead(&m_read);
                }

                void OnWriteDone(bool ok) override 
                {
                    ENSURE(ok);

                    std::lock_guard lock{m_write_mutex};
                    ENSURE(!m_write.empty());
                    m_write.pop_front();

                    if (!m_write.empty()) {
                        StartWrite(&m_write.front());
                    }
                }

                void OnDone(const grpc::Status& /*s*/) override
                {
                    m_disposable.dispose();
                    m_observer.on_completed();
                    delete this;
                }

            private:
                rpp::dynamic_observer<ChatService::Proto::Event> m_observer;
                rpp::disposable_wrapper                          m_disposable;

                ChatService::Proto::Event m_read{};

                std::mutex                                     m_write_mutex{};
                std::deque<ChatService::Proto::Event::Message> m_write{};
        };

        
    }

    Handler::Handler(const std::string& token)
        : m_state(InitState(m_context, token))
    {
    }

    Handler::State Handler::InitState(grpc::ClientContext& ctx, const std::string& token)
    {
        ctx.set_credentials(grpc::MetadataCredentialsFromPlugin(std::make_unique<Authenticator>(token)));

        rpp::subjects::serialized_subject<std::string>            messages{};
        rpp::subjects::publish_subject<ChatService::Proto::Event> events{};

        const auto reactor = new Reactor(messages.get_observable() | rpp::ops::map([](const std::string& txt) 
                                         {
                                             ChatService::Proto::Event::Message m{};
                                             m.set_text(txt);
                                             return m;
                                         }),
                                         events.get_observer().as_dynamic());
        ChatService::Proto::Server::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()))->async()->ChatStream(&ctx, reactor);
        reactor->Init();

        return {.events = events.get_observable().as_dynamic(), .messages = messages.get_observer().as_dynamic()};
    }

    Handler::~Handler() noexcept
    {
        m_state.messages.on_completed();
    }

    void Handler::SendMessage(const std::string& message) const
    {
        m_state.messages.on_next(message);
    }

    const rpp::dynamic_observable<ChatService::Proto::Event>& Handler::GetEvents() const
    {
        return m_state.events;
    }

}