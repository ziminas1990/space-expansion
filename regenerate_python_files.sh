#!/bin/bash

protoc --python_out=$(pwd)/python-sdk/expansion/protocol --proto_path=$(pwd)/server Protocol.proto Privileged.proto
