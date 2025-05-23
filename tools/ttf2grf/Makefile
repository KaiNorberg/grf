SRC = main.c
OBJ = ${SRC:.c=.o}

CC = gcc

CFLAGS = -O3 $(shell pkg-config --cflags freetype2)
LDFLAGS = $(shell pkg-config --libs freetype2)

all: ${SRC} ttf2grf

ttf2grf: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

${SRC}:
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	rm -f ttf2grf ${OBJ}