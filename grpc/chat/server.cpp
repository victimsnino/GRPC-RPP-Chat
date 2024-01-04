#include <chat.pb.h>
#include <chat.grpc.pb.h>
#include <grpc++/server_builder.h>

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

int main()
{
    ChatServer service{};
    grpc::ServerBuilder builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}