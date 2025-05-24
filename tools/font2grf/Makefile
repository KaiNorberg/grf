SRC = main.c
OBJ = ${SRC:.c=.o}

CC = gcc

CFLAGS = -O3 $(shell pkg-config --cflags freetype2) $(shell pkg-config --cflags harfbuzz)
LDFLAGS = $(shell pkg-config --libs freetype2) $(shell pkg-config --libs harfbuzz)

all: ${SRC} font2grf

font2grf: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

${SRC}:
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	rm -f font2grf ${OBJ}