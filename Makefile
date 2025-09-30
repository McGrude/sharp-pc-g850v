# BSD make Makefile for Sharp PC-G850V tools
# Usage:
#   make                    # build all
#   make clean
#   make RECV_SRC=recvfile.c

# Toolchain (FreeBSD: /usr/bin/cc -> clang)
CC?=cc
CFLAGS?=-Wall -Wextra -O2
LDFLAGS?=
TERMINAL_LIBS=-pthread -lncurses -lform

# If you renamed the receiver source, override on the command line:
#   make RECV_SRC=recvfile.c
RECV_SRC?=recvfile.c
RECV_OBJ=${RECV_SRC:R}.o

# Default target
all: sendfile recvfile terminal

# Suffix rules
.SUFFIXES: .c .o
.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

# Programs
sendfile: sendfile.o xfer.o
	${CC} ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}

recvfile: ${RECV_OBJ} xfer.o
	${CC} ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}

terminal: terminal.o xfer.o
	${CC} ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC} ${TERMINAL_LIBS}

# Objects that depend on the serial header
sendfile.o: sendfile.c xfer.h
terminal.o: terminal.c xfer.h
${RECV_OBJ}: ${RECV_SRC} xfer.h
xfer.o: xfer.c xfer.h

clean:
	rm -f sendfile recvfile terminal *.o
