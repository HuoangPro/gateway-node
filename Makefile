.PHONY: all clean gateway node

INCLUDE_DIR = include
SOURCE_DIR = src
BUILD_DIR = build
CXXLDFLAGS += -lpthread -lrt
CXXFLAGS += -std=c++17

GATEWAY_SRC := ${SOURCE_DIR}/gateway.cpp
NODE_SRC := ${SOURCE_DIR}/node.cpp

all: dir gateway node

dir: 
	mkdir -p $(BUILD_DIR)

gateway: ${GATEWAY_SRC}
	g++ -o $(BUILD_DIR)/gateway ${GATEWAY_SRC} -I${INCLUDE_DIR} ${CXXLDFLAGS} ${CXXFLAGS}

node: ${NODE_SRC}
	g++ -o $(BUILD_DIR)/node ${NODE_SRC} -I${INCLUDE_DIR} ${CXXLDFLAGS} ${CXXFLAGS}

clean: 
	rm -rf *.o gateway node $(BUILD_DIR)/gateway $(BUILD_DIR)/node $(BUILD_DIR)/*.o

cleanall: clean
	rm -rf log storage $(BUILD_DIR)