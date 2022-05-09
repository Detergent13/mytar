CC = gcc
CFLAGS = -Wall -pedantic -g

mytar: mytar.c
	$(CC) $(CFLAGS) -o mytar mytar.c
all: mytar
test: mytar
	./mytar
clean:
