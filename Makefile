.PHONY: clean, mrproper

CC = gcc
CPPC = g++
CFLAGS = -Wall -std=c++11 $(shell llvm-config --cppflags --libs core jit native) -g 
LDFLAGS = -lstdc++ $(shell llvm-config --ldflags --libs core jit native) -rdynamic
OutFile = pifc
Objects = src/main.o src/Package.o src/prettyprint.o src/util.o \
		src/lexer/Lexer.o  \
		src/parser/type.o src/parser/expr.o src/parser/stmt.o \
		src/typecheck/types.o \
		src/codegen-llvm/expr.o src/codegen-llvm/type.o src/codegen-llvm/Generator.o \

all: $(Objects)
	$(CC) $^ -o $(OutFile) $(LDFLAGS)

rebuild: mrproper all

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.cpp
	$(CPPC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf src/*.o
	rm -rf src/*/*.o

mrproper: clean
	rm -rf $(OutFile)

r3: clean
	make -j3
