CC = gcc
CFLAGS = -Wall -lpthread -lrt -g

all: 
	$(CC) $(CFLAGS) -o assign2  assign2_template-v4.c  
