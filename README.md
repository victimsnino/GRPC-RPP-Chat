# GRPC-RPP-Chat

Example implementatiuon of RPC chat with server/client, authenication and etc using GGRPC for RPC communication and RPP for internal streaming logic

## Example:

![gif](./media/grpc_rpp_chat.gif)

## Implementation:

Both (server and client) uses grpc streaming inside. But instead of raw using grpc streams they are wrap them into RPP observables/observers so it can be easily piped to make any complex streams of data.