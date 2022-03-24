CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve


sudoku_solve: main-us.cc sudoku_dancing_links.cc -lpthread
	g++ -g -Wall -o $@ $^

clean: 
	rm -rf sudoku_solve
