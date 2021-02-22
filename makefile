CC = gcc
CFLAGS = -Wall
LIB = 

SOURCE = sandbox.c

sandbox: ${SOURCE}
	${CC} ${SOURCE} ${CFLAGS} ${LIB} -o sandbox

debug: ${SOURCE}
	${CC} ${SOURCE} ${CFLAGS} -g -DDEBUG ${LIB} -o sandbox

clean:
	rm sandbox