#!/bin/bash

set -e

# Create output folder
mkdir -p build

# Compile server
g++ -std=c++17 examples/fibo_server.cpp server.cpp transport.cpp schema.cpp threadpool.cpp client.cpp -I. -o build/fibo_server

# Compile client
g++ -std=c++17 examples/fibo_client.cpp server.cpp transport.cpp schema.cpp threadpool.cpp client.cpp -I. -o build/fibo_client

echo "Build successful. Binaries are in ./build/"
