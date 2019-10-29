CC = gcc
CFLAGS = -g -rdynamic -Wall
SRC = *.c
OUTPUT = server

build: ${SRC}
		${CC} ${CFLAGS} $^ -o ${OUTPUT}

clean:
	rm -f ${OUTPUT}

run:
	./${OUTPUT}

debug:
	gdb ${OUTPUT}

all: clean build run

rd: clean build debug
