// sendfile.c
// Sharp PC-G850V SendFile Utility (throttled + XON/XOFF)
// - Opens serial via init_fd() from xfer.h
// - Sends file line-by-line, normalizes to CRLF
// - Throttles output with small chunk writes + tcdrain() + short sleeps
// - Enables software flow control (IXON/IXOFF/IXANY) so the G850V can XOFF us
// - Appends CP/M EOF (0x1A) at the end

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>

#include "xfer.h"

/* -------- Tunables (can override via -D on the compile line) -------- */
#ifndef SEND_CHUNK_BYTES
#define SEND_CHUNK_BYTES 32       /* bytes per burst */
#endif

#ifndef SEND_CHUNK_US
#define SEND_CHUNK_US    3000     /* µs to sleep after each burst (~3 ms) */
#endif

#ifndef SEND_AFTER_LINE_US
#define SEND_AFTER_LINE_US 10000  /* µs to sleep after each CRLF (~10 ms) */
#endif
/* -------------------------------------------------------------------- */

static void nap_us(useconds_t us) {
    if (us) (void)usleep(us);
}

/* Enable or disable software flow control (XON/XOFF). Returns 0 on success. */
static int set_sw_flowcontrol(int fd, int enable) {
    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) return -1;

    if (enable) {
        tio.c_iflag |= (IXON | IXOFF | IXANY);
    } else {
        tio.c_iflag &= ~(IXON | IXOFF | IXANY);
    }
    return tcsetattr(fd, TCSANOW, &tio);
}

/* write_all_chunked: send [buf,len] in small bursts with tcdrain() + naps */
static int write_all_chunked(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        size_t left = len - off;
        size_t nsend = left < SEND_CHUNK_BYTES ? left : SEND_CHUNK_BYTES;

        ssize_t n = write(fd, buf + off, nsend);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)n;

        /* Block until the kernel has pushed bytes to the wire */
        if (tcdrain(fd) < 0) return -1;

        /* Brief pause so the G850V can service its RX buffer */
        nap_us(SEND_CHUNK_US);
    }
    return 0;
}

static int send_line(int fd, const char *line, size_t len) {
    /* Content */
    if (len > 0) {
        if (write_all_chunked(fd, line, len) != 0) return -1;
    }
    /* CRLF */
    static const char crlf[] = "\r\n";
    if (write_all_chunked(fd, crlf, 2) != 0) return -1;

    /* Per-line breather */
    nap_us(SEND_AFTER_LINE_US);
    return 0;
}

static int send_file(int fd, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error opening file for reading: %s\n", filename);
        return -1;
    }

    /* Read buffer for typical lines; very long lines are handled across multiple fgets() calls. */
    char buf[4096];

    while (fgets(buf, sizeof buf, fp)) {
        size_t len = strlen(buf);

        /* Normalize: strip trailing CR and/or LF */
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }

        /* Send this chunk as a "line" (even if the original line is longer than 4096) */
        if (send_line(fd, buf, len) != 0) {
            fclose(fp);
            return -1;
        }

        /* If the original line exceeded the buffer, continue flushing
           more characters until we hit the original EOL. */
        if (len == sizeof(buf) - 1 && buf[len - 1] != '\0') {
            int c;
            size_t chunk_len = 0;
            while ((c = fgetc(fp)) != EOF) {
                if (c == '\n') {
                    if (send_line(fd, "", 0) != 0) { fclose(fp); return -1; }
                    break;
                }
                if (c == '\r') {
                    int c2 = fgetc(fp);
                    if (c2 != '\n' && c2 != EOF) ungetc(c2, fp);
                    if (send_line(fd, "", 0) != 0) { fclose(fp); return -1; }
                    break;
                }
                /* Build a small temp buffer to avoid syscalls per char */
                char tmp[256];
                tmp[chunk_len++] = (char)c;
                if (chunk_len == sizeof(tmp)) {
                    if (write_all_chunked(fd, tmp, chunk_len) != 0) { fclose(fp); return -1; }
                    chunk_len = 0;
                }
            }
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    /* Append CP/M EOF marker (0x1A) */
    unsigned char cpmeof = 0x1A;
    if (write_all_chunked(fd, (const char *)&cpmeof, 1) != 0) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fputs(
            "SHARP PC-G850V SendFile Utility (throttled, XON/XOFF)\n"
            "Usage: ./sendfile <serial-port> <filename>\n", stderr);
        return 1;
    }

    int fd = init_fd(argv[1]);
    if (fd < 0) {
        return 1;
    }

    /* Enable software flow control so the G850V can XOFF us if needed */
    if (set_sw_flowcontrol(fd, 1) != 0) {
        fputs("Warning: failed to enable XON/XOFF; continuing with throttling only.\n", stderr);
    }

    int rc = send_file(fd, argv[2]);
    if (rc < 0) {
        fputs("Failed to send file\n", stderr);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
