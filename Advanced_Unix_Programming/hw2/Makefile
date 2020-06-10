CC = gcc
CFLAGS = -I. -Wall -s

.PHONY: hw2 clean

hw2: main.o opt.o listdir.o path.o proc.o
	$(CC) -static -o hw2 $^ $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)


clean:
	rm hw2 *.o
