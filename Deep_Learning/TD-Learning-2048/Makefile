CXX = g++
#CFLAGS = -I. -Wall -std=c++11 -O0
# CFLAGS = -std=c++0x -O3 -g -Wall -fmessage-length=0
CFLAGS = -std=c++0x -O3 -g -Wall -fmessage-length=0 -DAI_JS_OUTPUT

OBJS = state.o game.o agent.o tuple_network.o statistic.o

.PHONY: clean

all: main

exp:  main_experiment.o $(OBJS)
	$(CXX) $(CFLAGS) $^ -o exp

main: main.o $(OBJS)
	$(CXX) $(CFLAGS) $^ -o main

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	rm $(OBJS) main main_exp

test:
	$(CXX) $(CFLAGS) test.cpp
