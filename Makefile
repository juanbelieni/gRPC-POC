PROTO_DIR := ./proto
BUILD_DIR := ./build

generate-cpp:
	protoc -I. \
		--cpp_out=. \
		--grpc_out=. \
		--plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
		$(PROTO_DIR)/*.proto

generate-python:
	protoc -I. \
		--python_out=. \
		--grpc_out=. \
		--plugin=protoc-gen-grpc=`which grpc_python_plugin` \
		$(PROTO_DIR)/*.proto

generate: generate-cpp generate-python

build-server:
	@mkdir -p $(BUILD_DIR)
	g++ -std=c++20 \
		`pkg-config --cflags --libs protobuf grpc++` \
		-I. \
		-I$(PROTO_DIR) \
		$(PROTO_DIR)/*.cc \
		server.cpp \
		-o $(BUILD_DIR)/server

start-server:
	$(BUILD_DIR)/server

start-client:
	python client.py
