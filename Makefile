.PHONY: all clean 

all: build run

build:
	gcc -fopenmp -o bin/main cache_sim.c
run:
	bin/main
debug:
	gcc -g -DDEBUG -fopenmp -o bin/main cache_sim.c
