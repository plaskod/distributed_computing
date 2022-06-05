
SOURCES=$(wildcard *.cc)
HEADERS=$(SOURCES:.cc=.hh)
FLAGS=-DDEBUG -g

all: main

main: $(SOURCES) $(HEADERS)
	mpic++ $(SOURCES) $(FLAGS) -o main

clean:
	rm main

run: main
	mpirun -np 3 --oversubscribe ./main

# rerun: $(SOURCES) $(HEADERS)
# 	rm main a.out
# 	mpic++ $(SOURCES) $(FLAGS) -o main
# 	mpirun -np 2 --oversubscribe ./main

