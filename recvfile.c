// recvfile.c
// Sharp PC-G850V RecvFile Utility
// - Reads from serial fd until EOF or CP/M EOF (0x1A).
// - Normalizes line endings to '\n'.
// - If a line begins with digits (after optional spaces),
//   inserts a space immediately after the digits.
// - Depends on xfer.h / xfer.o for init_fd() and serial setup.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "xfer.h"

/* Write all bytes to FILE* */
static int write_all(FILE *fp, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        size_t n = fwrite(buf + off, 1, len - off, fp);
        if (n == 0) return -1;
        off += n;
    }
    return 0;
}

/* Insert a space immediately after a leading digit sequence, if present. */
static void insert_space_after_digits(const char *in, size_t in_len,
                                      char *out, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    // Copy leading spaces (unchanged)
    while (i < in_len && in[i] == ' ') {
        out[j++] = in[i++];
    }

    // Copy digits
    size_t digit_start = i;
    while (i < in_len && isdigit((unsigned char)in[i])) {
        out[j++] = in[i++];
    }

    if (i > digit_start) {
        // Insert an extra space before next non-digit if not already space/tab
        if (i < in_len && in[i] != ' ' && in[i] != '\t') {
            out[j++] = ' ';
        }
    }

    // Copy the rest
    while (i < in_len) {
        out[j++] = in[i++];
    }

    *out_len = j;
}

/*
 * recv_file:
 *  - Reads from fd, writes to filename
 *  - Handles CRLF/CR/LF, turns them into '\n'
 *  - Inserts a space after any leading digits at line start
 *  - Stops on EOF or CP/M 0x1A marker
 */
static int recv_file(int fd, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing\n");
        return -1;
    }

    char line_buf[65536];
    size_t line_len = 0;

    char in_buf[32768];
    ssize_t nread;
    int seen_cpmeof = 0;
    int last_was_cr = 0;

    while ((nread = read(fd, in_buf, sizeof in_buf)) > 0) {
        for (ssize_t i = 0; i < nread; ++i) {
            unsigned char ch = (unsigned char)in_buf[i];

            if (ch == 0x1A) { seen_cpmeof = 1; break; }

            if (last_was_cr) {
                last_was_cr = 0;
                if (ch == '\n') {
                    char out_line[65536*2]; size_t out_len;
                    insert_space_after_digits(line_buf, line_len, out_line, &out_len);
                    if (write_all(fp, out_line, out_len) < 0 || fputc('\n', fp) == EOF) {
                        fclose(fp);
                        return 1;
                    }
                    line_len = 0;
                    continue;
                } else {
                    char out_line[65536*2]; size_t out_len;
                    insert_space_after_digits(line_buf, line_len, out_line, &out_len);
                    if (write_all(fp, out_line, out_len) < 0 || fputc('\n', fp) == EOF) {
                        fclose(fp);
                        return 1;
                    }
                    line_len = 0;
                }
            }

            if (ch == '\r') {
                last_was_cr = 1;
            } else if (ch == '\n') {
                char out_line[65536*2]; size_t out_len;
                insert_space_after_digits(line_buf, line_len, out_line, &out_len);
                if (write_all(fp, out_line, out_len) < 0 || fputc('\n', fp) == EOF) {
                    fclose(fp);
                    return 1;
                }
                line_len = 0;
            } else {
                if (line_len < sizeof(line_buf)) {
                    line_buf[line_len++] = (char)ch;
                } else {
                    // Flush long line
                    char out_line[65536*2]; size_t out_len;
                    insert_space_after_digits(line_buf, line_len, out_line, &out_len);
                    if (write_all(fp, out_line, out_len) < 0) {
                        fclose(fp);
                        return 1;
                    }
                    line_len = 0;
                    line_buf[line_len++] = (char)ch;
                }
            }
        }
        if (seen_cpmeof) break;
    }

    if (nread < 0) {
        fclose(fp);
        return 1;
    }

    // Flush final line if any
    if (last_was_cr || line_len > 0) {
        char out_line[65536*2]; size_t out_len;
        insert_space_after_digits(line_buf, line_len, out_line, &out_len);
        if (write_all(fp, out_line, out_len) < 0) {
            fclose(fp);
            return 1;
        }
        if (last_was_cr) {
            if (fputc('\n', fp) == EOF) {
                fclose(fp);
                return 1;
            }
        }
    }

    if (fclose(fp) != 0) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    int fd;
    if (argc != 3) {
        fputs(
            "SHARP PC-G850V RecvFile Utility\n"
            "Usage: ./recvfile port filename\n", stderr);
        return 1;
    }

    if ((fd = init_fd(argv[1])) < 0) {
        return 1;
    }

    if (recv_file(fd, argv[2]) < 0) {
        fputs("Failed to receive file\n", stderr);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}


