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

        template<rpp::constraint::decayed_type Request, rpp::constraint::decayed_type Response>
        class client_bidi_reactor final : public grpc::ClientBidiReactor<Request, Response>
        {
            using Base = grpc::ClientBidiReactor<Request, Response>;

        public:
            client_bidi_reactor()
            {
                m_requests.get_observable().subscribe(
                    [this]<rpp::constraint::decayed_same_as<Request> T>(T&& message) {
                        std::lock_guard lock{m_write_mutex};
                        m_write.push_back(std::forward<T>(message));
                        if (m_write.size() == 1)
                            Base::StartWrite(&m_write.front());
                    },
                    [this](const std::exception_ptr&) {
                        Base::StartWritesDone();
                    },
                    [this]() {
                        Base::StartWritesDone();
                    });
            }

            void init()
            {
                Base::StartCall();
                Base::StartRead(&m_read);
            }

            auto get_observer()
            {
                return m_requests.get_observer();
            }

            auto get_observable()
            {
                return m_observer.get_observable();
            }

        private:
            using Base::StartCall;
            using Base::StartRead;

            void OnReadDone(bool ok) override
            {
                if (!ok)
                {
                    m_observer.get_observer().on_error(std::make_exception_ptr(std::runtime_error{"OnReadDone is not ok"}));
                    return;
                }
                m_observer.get_observer().on_next(m_read);
                Base::StartRead(&m_read);
            }

            void OnWriteDone(bool ok) override
            {
                if (!ok)
                {
                    m_requests.get_disposable().dispose();
                    m_observer.get_observer().on_error(std::make_exception_ptr(std::runtime_error{"OnWriteDone is not ok"}));
                    return;
                }

                std::lock_guard lock{m_write_mutex};
                m_write.pop_front();

                if (!m_write.empty())
                {
                    Base::StartWrite(&m_write.front());
                }
            }

            void OnDone(const grpc::Status& s) override
            {
                std::cout << "ON DONE " << s.error_code() << std::endl;
                m_requests.get_disposable().dispose();
                if (s.ok())
                {
                    m_observer.get_observer().on_completed();
                }
                else
                {
                    m_observer.get_observer().on_error(std::make_exception_ptr(std::runtime_error{s.error_message()}));
                }
                delete this;
            }

        private:
            rpp::subjects::serialized_publish_subject<Request> m_requests{};

            rpp::subjects::publish_subject<Response> m_observer;
            Response                                 m_read{};

            std::mutex          m_write_mutex{};
            std::deque<Request> m_write{};
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

            const auto reactor = new client_bidi_reactor<ChatService::Proto::Event::Message, ChatService::Proto::Event>();
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