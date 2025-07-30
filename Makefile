CC = gcc
CFLAGS = -Wall -Wextra -O2
ENTRY = SincoMain

all: sinco

sinco: main.o elf.o syscall.o
	@$(CC) $(CFLAGS) -o sincoc main.o elf.o syscall.o

main.o: main.c elf.h syscall.h
	@$(CC) $(CFLAGS) -c main.c

elf.o: elf.c elf.h
	@$(CC) $(CFLAGS) -c elf.c

syscall.o: syscall.c syscall.h
	@$(CC) $(CFLAGS) -c syscall.c

clean:
	@del .\\*.o .\\sincoc.exe