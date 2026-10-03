CC       = gcc
CXX      = g++
CFLAGS   = -O2 -Wall -Wextra -Idsa
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread -Idsa
DSA_SRC  = $(wildcard dsa/*.c)
DSA_OBJ  = $(filter-out obj/test_dsa.o,$(patsubst dsa/%.c,obj/%.o,$(DSA_SRC)))

all: bin/gateway bin/backend

obj/%.o: dsa/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

bin/gateway: src/gateway.cpp src/net.hpp $(DSA_OBJ)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) src/gateway.cpp $(DSA_OBJ) -o $@

bin/backend: src/backend.cpp src/net.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) src/backend.cpp -o $@

bin/test_dsa: dsa/test_dsa.c $(DSA_OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) dsa/test_dsa.c $(DSA_OBJ) -o $@

test: bin/test_dsa
	./bin/test_dsa

clean:
	rm -rf obj bin .run
.PHONY: all test clean
