CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -std=c++17 -O2
LIBS = -lsndfile -ldl
BINARY = audiomaster

.PHONY: all
all: $(BINARY)


$(BINARY): audiomaster.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)
