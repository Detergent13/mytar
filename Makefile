CC = gcc
CFLAGS = -Wall -pedantic -g

all: mytar

mytar: mytar.o create.o list.o extract.o util.o given.o
    $(CC) $(CFLAGS) -o mytar mytar.o create.o list.o extract.o util.o given.o

mytar.o: mytar.c
    $(CC) $(CFLAGS) -c mytar.c

create.o: create.c
    $(CC) $(CFLAGS) -c create.c

list.o: list.c
    $(CC) $(CFLAGS) -c list.c

extract.o: extract.c
    $(CC) $(CFLAGS) -c extract.c

util.o: util.c
    $(CC) $(CFLAGS) -c util.c

given.o: given.c
    $(CC) $(CFLAGS) -c given.c

test: mytar
    ./mytar

clean:

