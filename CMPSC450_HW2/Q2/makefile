CC=gcc
CFLAGS= -g -fpic -std=gnu99
#TARGETS=project2
OBJFILES=cmpsc473mr.o main.o

all: project clean

#cmpsc473mr.o: cmpsc473mr.c
#	$(CC) -pthread -c $(CFLAGS) $^

main.o: cmpsc473mr.c
	$(CC) -pthread -c $^ $(CFLAGS)

project: cmpsc473mr.o
	$(CC) -pthread -o $@ $^ $(CFLAGS)

# Cleanup 
clean:
	rm -f $(OBJFILES) 
