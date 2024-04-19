.PHONY all clean 

all: build run

build:
	gcc -o bin/main cache_sim.c
run:
	bin/main
debug:
	gcc -g -DDEBUG -o bin/main cache_sim.c
