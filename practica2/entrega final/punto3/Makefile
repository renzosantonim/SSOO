#Makefile Renzo Santoni Moyano

SHELL := /bin/bash
TARGET = docserver
CXX = g++
CXXFLAGS = -std=c++23 -O1 -o $(TARGET)
######################################
files = ./*.cc
######################################
all: $(TARGET)

$(TARGET): $(files)
	@$(CXX) $(CXXFLAGS) $(files)

######################################
.PHONY: clean
clean:
	@echo "Cleaning up the directory..."
	@ls | grep -v "Makefile" | grep -v ".cc" | grep -v ".h" | grep -v ".txt" | xargs rm -rf
######################################
.PHONY: help
help:
	@echo "make -> compile"
	@echo "make clean -> remove all non [.cc/.h/.txt/Makefile] files"
	
######################################