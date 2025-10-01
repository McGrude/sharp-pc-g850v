# Portable Makefile for Sharp PC-G850V tools
# Works with GNU make (macOS default) and BSD make (FreeBSD bmake) by using
# explicit compile/link commands only.

CC?=cc
CFLAGS?=-Wall -Wextra -O2
LDFLAGS?=
TERMINAL_LIBS?=-pthread -lncurses -lform

all: sendfile recvfile terminal

# --- Link rules ---
sendfile: sendfile.o xfer.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o sendfile sendfile.o xfer.o

recvfile: recvfile.o xfer.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o recvfile recvfile.o xfer.o

terminal: terminal.o xfer.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o terminal terminal.o xfer.o $(TERMINAL_LIBS)

# --- Compile rules (explicit) ---
sendfile.o: sendfile.c xfer.h
	$(CC) $(CFLAGS) -c sendfile.c -o sendfile.o

recvfile.o: recvfile.c xfer.h
	$(CC) $(CFLAGS) -c recvfile.c -o recvfile.o

xfer.o: xfer.c xfer.h
	$(CC) $(CFLAGS) -c xfer.c -o xfer.o

terminal.o: terminal.c xfer.h
	$(CC) $(CFLAGS) -c terminal.c -o terminal.o

# --- Housekeeping ---
.PHONY: all clean
clean:
	rm -f sendfile recvfile terminal *.o

