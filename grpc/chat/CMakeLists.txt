find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

macro(add_proto_library TARGET FILES)
    file(RELATIVE_PATH RELATIVE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    add_library(${TARGET} STATIC ${FILES})

    target_link_libraries(${TARGET}
        PUBLIC
            gRPC::grpc++
            protobuf::libprotobuf
    )

    set(PROTO_BINARY_DIR ${CMAKE_BINARY_DIR}/generated/${RELATIVE_PATH})
    make_directory(${PROTO_BINARY_DIR})

    target_include_directories(${TARGET} PUBLIC ${PROTO_BINARY_DIR})

    get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION )
    protobuf_generate(TARGET ${TARGET} OUT_VAR PROTO_FILES LANGUAGE cpp PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

    protobuf_generate(
        TARGET ${TARGET}
        LANGUAGE grpc
        OUT_VAR GRPC_PROTO_FILES
        PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
        PLUGIN protoc-gen-grpc=${grpc_cpp_plugin_location}
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc)
    
    target_sources(${TARGET} PUBLIC ${PROTO_FILES} ${GRPC_PROTO_FILES})
endmacro()


add_proto_library(proto chat.proto)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE gRPC::grpc++ proto)
set_target_properties(server PROPERTIES FOLDER grpc/chat)

add_executable(client client.cpp)
target_link_libraries(client PRIVATE gRPC::grpc++ proto)
set_target_properties(client PROPERTIES FOLDER grpc/chat)