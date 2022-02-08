#!/bin/bash


protoc -I$(pwd)/python-sdk --python_out=$(pwd)/python-sdk/ --proto_path=$(pwd)/server CommonTypes.proto Protocol.proto Privileged.proto
