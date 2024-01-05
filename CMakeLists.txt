cmake_minimum_required(VERSION 3.14)

project(
    PracticeProjects
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###################################### OPTIONS ##################################
option(BUILD_TESTS      "Build unit tests tree." ON)

macro(add_3rdparty TARGET REPO TAG)
    Include(FetchContent)

    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build SHARED libraries")

    FetchContent_Declare(${TARGET}
        GIT_REPOSITORY ${REPO}
        GIT_TAG        ${TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(${TARGET})

    # target_compile_options(${TARGET} PRIVATE "-w")
    set_target_properties(${TARGET} PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(${TARGET} PROPERTIES CXX_CPPCHECK "")
    set_target_properties(${TARGET} PROPERTIES FOLDER 3rdparty)
endmacro()
#################################################################################


find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_3rdparty(jwt-cpp https://github.com/Thalhammer/jwt-cpp v0.7.0)

if (BUILD_TESTS)
  add_3rdparty(snitch https://github.com/cschreib/snitch.git main)
endif()

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
    
    if (NOT TARGET custom_build_all_proto)
        add_custom_target(custom_build_all_proto ALL)
    endif()
    add_dependencies(custom_build_all_proto ${TARGET})
    
    # target_sources(${TARGET} PUBLIC ${PROTO_FILES} ${GRPC_PROTO_FILES})
endmacro()


add_subdirectory(src)