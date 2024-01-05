#include "chat_service.hpp"


namespace ChatService 
{
    grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>* Service::ChatStream(grpc::CallbackServerContext* ctx)
    {
        class Result final : public grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>
        {
        public:
            void OnReadDone(bool /*ok*/) override 
            {

            }

            void OnWriteDone(bool /*ok*/) override 
            {

            }

            void OnDone() override 
            {
                delete this;
            }

        private:
        };
        return new Result();
    }
}
