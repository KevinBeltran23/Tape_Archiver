CC = gcc

CFLAGS = -g

LD = gcc

LDFLAGS = -lm

.SILENT: 

all: mytar

mytar: tar.o
	$(LD) $(LDFLAGS) -o mytar tar.o

tar.o: tar.c
	$(CC) $(CFLAGS) -c -o tar.o tar.c

test:
	valgrind --leak-check=yes ./mytar cf tartest1 JustAFile dirpath JustAnotherFile

testc: 
	valgrind --leak-check=yes ./mytar cvf tartest1 JustAFile dirpath JustAnotherFile

testx:
	valgrind --leak-check=yes ./mytar xvf tartest1 

testt:
	valgrind --leak-check=yes ./mytar tvf tartest1 
clean:
	rm -f *.o mytar
