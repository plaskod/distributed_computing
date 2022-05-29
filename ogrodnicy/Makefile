
SOURCES=$(wildcard *.cc)
HEADERS=$(SOURCES:.cc=.hh)
FLAGS=-DDEBUG -g

all: main

main: $(SOURCES) $(HEADERS)
	mpic++ $(SOURCES) $(FLAGS) -o main

clean:
	rm main a.out

run: main
	mpirun -np 8 --oversubscribe ./main
