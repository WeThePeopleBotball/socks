cmake_minimum_required(VERSION 3.14)
project(socks_lib LANGUAGES CXX)

# Library source files
set(LIB_SOURCES
    client.cpp
    server.cpp
    schema.cpp
    threadpool.cpp
    transport.cpp
)

# Library header files (optional, for IDEs)
set(LIB_HEADERS
    client.hpp
    server.hpp
    schema.hpp
    threadpool.hpp
    transport.hpp
)

# Define the static library target
add_library(socks STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(socks PUBLIC ${JSON_LIB})

# Set warnings
target_compile_options(socks PRIVATE -Wall -Wextra -pedantic)

# Link pthread for std::thread and async usage
find_package(Threads REQUIRED)
target_link_libraries(socks PUBLIC Threads::Threads)

# Option to build examples
option(SOCKS_BUILD_EXAMPLES "Build example applications" OFF)

if(SOCKS_BUILD_EXAMPLES)
    add_executable(fibo_client examples/fibo_client.cpp)
    target_link_libraries(fibo_client socks)

    add_executable(fibo_server examples/fibo_server.cpp)
    target_link_libraries(fibo_server socks)

    set_target_properties(fibo_client fibo_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()
