#include <chat.pb.h>
#include <chat.grpc.pb.h>
#include <grpc++/server_builder.h>

#include "common.h"

class ChatServer final : public Server::Service {
public:
    ::grpc::Status ChatStream(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::Message, ::Message_Text>* stream) override {

        Message::Text input{};
        while(stream->Read(&input)) {
            std::cout << "GOT " << input.text() << std::endl;

            Message output{};
            input.set_text(input.text()+ " REPLY");
            *output.mutable_text() = input;
            stream->Write(output);
        }
        return grpc::Status::OK;
    }
};

void RunSyncServer() {
    ChatServer service{};
    grpc::ServerBuilder builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

void RunAsyncServer () {
    Server::AsyncService service;
    grpc::ServerBuilder builder;
    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto cq = builder.AddCompletionQueue();
    auto server = builder.BuildAndStart();

    std::cout << "Async Server listening on " << server_address << std::endl;

    auto ctx = std::make_unique<grpc::ServerContext>();
    grpc::ServerAsyncReaderWriter< ::Message, ::Message_Text> stream{ctx.get()};
    service.RequestChatStream(ctx.get(), &stream, cq.get(), cq.get(), reinterpret_cast<void*>(size_t{1}));

    void* got_tag{};
    bool ok{};
    Message::Text input{};
    Message output{};
    while(cq->Next(&got_tag, &ok)) {
        const size_t tag = reinterpret_cast<size_t>(got_tag);

        std::cout << ok << " " << tag << std::endl;
        switch(tag) {
            case 1:
                stream.Read(&input, reinterpret_cast<void*>(size_t{2}));
                break;
            case 2:
                std::cout << input.text() << std::endl;
                output.mutable_text()->set_text("12323333");
                stream.WriteAndFinish(output, {}, grpc::Status::OK, reinterpret_cast<void *>(size_t{4}));
                break;
            case 4:
                ctx = std::make_unique<grpc::ServerContext>();
                stream = grpc::ServerAsyncReaderWriter< ::Message, ::Message_Text>{ctx.get()};
                service.RequestChatStream(ctx.get(), &stream, cq.get(), cq.get(), reinterpret_cast<void*>(size_t{1}));
                break;
            default:
                break;
        }

    }
}

class CallbackServer final : public Server::CallbackService {
public:
    ::grpc::ServerBidiReactor< ::Message_Text, ::Message>* ChatStream(::grpc::CallbackServerContext* context) override {
        std::cout << "NEW CONNECTION " << std::endl;
        class Result final : public grpc::ServerBidiReactor< ::Message_Text, ::Message> {
            public:
                Result() 
                {
                    StartRead(data.mutable_text());
                    StartWrite(&data);
                }
                
            private:
                void OnDone() override 
                { 
                    std::cout << "DONE" << std::endl;
                    delete this;
                }

                void OnReadDone(bool ok) override {
                    if (ok)
                    {
                        std::cout << "Read " << data.text().text() << std::endl;
                        data.mutable_text()->set_text("response");
                        // StartWrite(&data);
                    } else {
                        std::cout << "FINISH " << std::endl;
                        Finish(grpc::Status::OK);
                    }
                }

                void OnWriteDone(bool /*ok*/) override {
                    std::cout << "Write done " << data.text().text() << std::endl;
                    StartRead(data.mutable_text());
                }

                void OnCancel() override {
                    std::cout << "CANCEL" << std::endl;
                }

            private:
                Message data{};
        };

        return new Result();
    }
};

void RunCallbackServer() {
    CallbackServer service;
    grpc::ServerBuilder builder;
    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto cq = builder.AddCompletionQueue();
    auto server = builder.BuildAndStart();

    std::cout << "Callback Server listening on " << server_address << std::endl;

    server->Wait();
}

int main()
{
    RunAsyncServer();

    return 0;
}