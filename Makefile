# Makefile for the MapGen Engine (Campus Locator and Navigation).
# Usage:
#   make            build the mapgen executable (console app)
#   make web        build the web server (mapgen_web)
#   make tests      build and run the self-test suite
#   make webtests   build and run the web API tests
#   make run        build and start the interactive menu
#   make serve      build and start the web server (http://localhost:18080)
#   make clean      remove build artifacts

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS  ?=

APP  := mapgen
WEB  := mapgen_web
TEST := mapgen_tests
WEBT := mapgen_web_tests

LIB_SRCS    := src/CampusMap.cpp src/MapGenerator.cpp src/Navigation.cpp src/Menu.cpp
LIB_OBJS    := $(LIB_SRCS:src/%.cpp=build/%.o)
WEB_SRCS    := src/web/HttpServer.cpp src/web/Json.cpp src/web/ApiController.cpp
WEB_OBJS    := $(WEB_SRCS:src/%.cpp=build/%.o)
TEST_SRCS   := tests/self_test.cpp
WEBT_SRCS   := tests/web_test.cpp

.PHONY: all tests web webtests run serve clean

all: $(APP) $(WEB)

build/%.o: src/%.cpp $(wildcard include/*.hpp) $(wildcard include/data_structures/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(WEB): $(LIB_OBJS) $(WEB_OBJS) build/web_main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST): $(TEST_SRCS) $(LIB_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) $(LIB_SRCS) -o $@

$(WEBT): $(WEBT_SRCS) $(LIB_SRCS) $(WEB_SRCS)
	$(CXX) $(CXXFLAGS) $(WEBT_SRCS) $(LIB_SRCS) $(WEB_SRCS) -o $@

tests: $(TEST)
	./$(TEST)

webtests: $(WEBT)
	./$(WEBT)

run: $(APP)
	./$(APP)

serve: $(WEB)
	./$(WEB)

clean:
	rm -rf build $(APP) $(WEB) $(TEST) $(WEBT)
