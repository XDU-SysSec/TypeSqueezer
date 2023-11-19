CC = gcc
CXX = g++ -std=c++11
CFLAGS = 
LINK = $(CXX)

ALL_INCS = main.h \
	output.h \
	initial.h \
	dynamic.h \
	direct.h

main : objs/initial.o \
	objs/output.o \
	objs/dynamic.o \
	objs/direct.o \
	objs/main.o
	$(LINK) -o main objs/initial.o \
	objs/output.o \
	objs/dynamic.o \
	objs/direct.o \
	objs/main.o


objs/direct.o: direct.cpp $(ALL_INCS)
	$(CXX) -c direct.cpp -o objs/direct.o

objs/initial.o: initial.cpp $(ALL_INCS)
	$(CXX) -c initial.cpp -o objs/initial.o

objs/dynamic.o: dynamic.cpp $(ALL_INCS)
	$(CXX) -c dynamic.cpp -o objs/dynamic.o

objs/output.o: output.cpp $(ALL_INCS)
	$(CXX) -c output.cpp -o objs/output.o

objs/main.o: main.cpp $(ALL_INCS)
	$(CXX) -c main.cpp -o objs/main.o
