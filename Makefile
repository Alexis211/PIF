.PHONY: clean, mrproper

CC = gcc
CPPC = g++
CFLAGS = -Wall -std=c++11 $(shell llvm-config --cppflags --libs core jit native) -g 
LDFLAGS = -lstdc++ $(shell llvm-config --ldflags --libs core jit native) -rdynamic
OutFile = pifc
Objects = src/main.o src/Lexer.o src/Package.o \
		src/Parser-type.o src/Parser-expr.o src/Parser-stmt.o \
		src/types.o src/codegen-expr.o src/codegen-type.o src/Generator.o src/prettyprint.o src/util.o

all: $(Objects)
	$(CC) $^ -o $(OutFile) $(LDFLAGS)

rebuild: mrproper all

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.cpp
	$(CPPC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf src/*.o

mrproper: clean
	rm -rf $(OutFile)

r3: clean
	make -j3
