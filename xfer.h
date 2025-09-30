// xfer.h
// Serial port setup helpers for Sharp PC-G850 (SIO mode).
// This header is meant to be included by programs that link against xfer.o,
// instead of including the C file directly.
//
// Build example:
//   gcc -Wall -O2 -c xfer.c -o xfer.o
//   gcc -Wall -O2 -o recvfile recvfile-fixed2.c xfer.o
//
// Notes:
// - set_interface_attribs() configures baud, parity, 8N1, etc.
// - set_blocking() toggles VMIN/VTIME to control blocking behavior.
// - init_fd() opens and configures the port (1200 baud by default) and returns the fd.

#ifndef XFER_H
#define XFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // size_t
#include <termios.h> // speed constants like B1200

// Configure serial attributes (speed/parity, 8N1).
// Returns 0 on success, -1 on error.
int set_interface_attribs(int fd, int speed, int parity);

// Control blocking behavior: should_block != 0 -> blocking, 0 -> non-blocking.
int set_blocking(int fd, int should_block);

// Open and initialize the serial port. Returns fd >= 0 on success, -1 on error.
int init_fd(char *port);

#ifdef __cplusplus
}
#endif

#endif /* XFER_H */
