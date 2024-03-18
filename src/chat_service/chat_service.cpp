#include "chat_service.hpp"

#include <rpp/rpp.hpp>
#include <jwt-cpp/jwt.h>

#include <common.hpp>


namespace ChatService
{
    namespace
    {
        class ReturnStatusReactor final : public grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>
        {
        public:
            explicit ReturnStatusReactor(grpc::Status s) { this->Finish(std::move(s)); }

            void OnDone() override { delete this; }
        };

        class Reactor final : public grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>
        {
            static auto InitObserver(const std::string& name, const rpp::dynamic_observer<UserObservable>& user_observables)
            {
                UserSubject subject{};
                user_observables.on_next(UserObservable{name, subject.get_observable()});
                return subject.get_observer();
            }

        public:
            Reactor(const std::string& name, const rpp::dynamic_observer<UserObservable>& user_observables, const rpp::dynamic_observable<Proto::Event>& all_events)
                : m_observer{InitObserver(name, user_observables)}
                , m_disposable{all_events
                               | rpp::ops::filter([name](const Proto::Event& user_events){return user_events.user() != name; })
                               | rpp::ops::subscribe_with_disposable([this](const Proto::Event& user_events) {
                                     std::lock_guard lock{m_write_mutex};
                                     m_write.push_back(user_events);
                                     if (m_write.size() == 1) {
                                        StartWrite(&m_write.front());
                                     }
                                 })}
            {
                StartRead(&m_read);
            }

        private:

            void OnReadDone(bool ok) override
            {
                if (!ok)
                {
                    Finish(grpc::Status::OK);
                    return;
                }

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

            void OnDone() override
            {
                m_disposable.dispose();
                m_observer.on_completed();
                delete this;
            }

        private:
            decltype(std::declval<UserSubject>().get_observer()) m_observer{};
            rpp::disposable_wrapper                              m_disposable{};

            Proto::Event::Message m_read{};

            std::mutex               m_write_mutex{};
            std::deque<Proto::Event> m_write{};
        };
    }

    Service::State Service::InitState()
    {
        rpp::subjects::serialized_publish_subject<UserObservable> subj{};

        auto events = subj.get_observable()
                    | rpp::ops::flat_map([](const UserObservable& observable) {
                          const auto messages = observable
                                              | rpp::ops::map([name = observable.get_key()](const Proto::Event::Message& message) {
                                                    Proto::Event event{};
                                                    event.set_user(name);
                                                    event.mutable_message()->CopyFrom(message);
                                                    return event;
                                                });

                          auto event = ChatService::Proto::Event{};
                          event.set_user(observable.get_key());
                          event.mutable_login();
                          const auto login_obs = rpp::source::just(event);

                          event.mutable_logout();
                          const auto logout_obs = rpp::source::just(event);
                          return rpp::source::concat(login_obs, messages, logout_obs);
                      })
                    | rpp::ops::publish();

        events.subscribe([](const Proto::Event& ev)
        {
            std::cout << ev.Utf8DebugString() << std::endl;
        });

        return State{.user_observables = subj.get_observer().as_dynamic(), .all_events = events, .disposable = events.connect()};
    }

    grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>* Service::ChatStream(grpc::CallbackServerContext* ctx)
    {
        const auto itr = ctx->client_metadata().find(Consts::g_authenication_header);
        if (itr == ctx->client_metadata().end())
            return new ReturnStatusReactor(grpc::Status{grpc::StatusCode::UNAUTHENTICATED, std::string{"No "} + Consts::g_authenication_header + " header"});

        const auto decoded = jwt::decode(std::string{itr->second.data(), itr->second.size()});

        std::error_code err{};
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{Consts::g_secret_seed})
            .with_issuer(Consts::g_issuer)
            .verify(decoded, err);

        if (err)
            return new ReturnStatusReactor(grpc::Status{grpc::StatusCode::UNAUTHENTICATED, std::string{"Invalid  token "} + err.message()});

        const auto payload = decoded.get_payload_json();
        const auto value_itr = payload.find(Consts::g_login_field);
        if (value_itr == payload.end()) {
            return new ReturnStatusReactor(grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "Ivalid  json payload"});
        }



        const auto new_handler = new Reactor(value_itr->second.get<std::string>(), m_state.user_observables, m_state.all_events);
        return new_handler;
    }
}
