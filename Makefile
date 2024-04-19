.PHONY: all clean 

all: build run

build:
	gcc -fopenmp -o bin/main.exe cache_sim.c
run:
	bin/main.exe
debug:
	gcc -g -DDEBUG -fopenmp -o bin/main.exe cache_sim.c
