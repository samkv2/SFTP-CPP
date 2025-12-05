cmake_minimum_required(VERSION 3.10)
project(SFTP-CPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

include_directories(include)

# Server
add_executable(sftp_server
    src/server/main.cpp
    src/server/server.cpp
    src/common/ssl_wrapper.cpp
    src/common/utils.cpp
)
target_link_libraries(sftp_server OpenSSL::SSL OpenSSL::Crypto pthread)
if(WIN32)
    target_link_libraries(sftp_server ws2_32)
endif()

# Client
add_executable(sftp_client
    src/client/main.cpp
    src/client/client.cpp
    src/common/ssl_wrapper.cpp
    src/common/utils.cpp
)
target_link_libraries(sftp_client OpenSSL::SSL OpenSSL::Crypto pthread)
if(WIN32)
    target_link_libraries(sftp_client ws2_32)
endif()
