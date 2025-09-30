// sendfile.c
// Sharp PC-G850V SendFile Utility (no line-number insertion)
//
// - Opens a serial port via init_fd() from xfer.h
// - Reads a text file line-by-line (fgets with buffer; multiple fgets handle very long lines)
// - Sends each line with CRLF line endings
// - Does NOT insert or manipulate line numbers
// - Appends CP/M EOF marker (0x1A) at the end (common for PC-G850 reception)
//
// Build (BSD make example):
//   cc -Wall -Wextra -O2 -c xfer.c -o xfer.o
//   cc -Wall -Wextra -O2 -c sendfile.c -o sendfile.o
//   cc -Wall -Wextra -O2 -o sendfile sendfile.o xfer.o

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "xfer.h"

static int write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}

static int send_file(int fd, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error opening file for reading: %s\n", filename);
        return -1;
    }

    char buf[4096];

    while (fgets(buf, sizeof buf, fp)) {
        size_t len = strlen(buf);

        // Remove any trailing CR and/or LF (normalize)
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }

        // Send the line content (could be empty)
        if (len > 0) {
            if (write_all(fd, buf, len) != 0) {
                fclose(fp);
                return -1;
            }
        }

        // Send CRLF after each chunk returned by fgets.
        // If the original line was longer than our buffer, this will emit CRLF
        // for each chunk. If you need strict "one CRLF per original line", a
        // streaming char-by-char approach would be necessary. For typical text
        // files this is fine.
        const char crlf[] = "\r\n";
        if (write_all(fd, crlf, 2) != 0) {
            fclose(fp);
            return -1;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    // Append CP/M EOF marker (0x1A)
    unsigned char cpmeof = 0x1A;
    if (write_all(fd, &cpmeof, 1) != 0) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fputs(
            "SHARP PC-G850V SendFile Utility\n"
            "Usage: ./sendfile port filename\n", stderr);
        return 1;
    }

    int fd = init_fd(argv[1]);
    if (fd < 0) {
        return 1;
    }

    if (send_file(fd, argv[2]) < 0) {
        fputs("Failed to send file\n", stderr);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
