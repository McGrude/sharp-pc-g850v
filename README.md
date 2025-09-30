# Sharp PC-G850V Serial Tools

Small utilities for moving text files between a host computer and a **Sharp PC‑G850 series** pocket computer using **SIO mode** over a serial port.

- `sendfile` — send a text file to the Sharp
- `recvfile` — receive a text file from the Sharp
- `xfer.c` / `xfer.h` — serial port setup helpers (open and configure the TTY)
- `terminal` — **TBD**

> These tools aim to be simple and predictable: they do not do protocol handshakes or “smart” conversions, and they do not insert or strip BASIC line numbers unless specifically noted below.

---

## Build

### FreeBSD / BSD make (default `cc` = clang)

A BSD-make–compatible `Makefile` is provided. If you are using the version that allows choosing the receiver source, you can build like this:

```sh
# Clean build
make clean

make
```

This compiles:
- `xfer.c` → `xfer.o`
- `sendfile.c` → `sendfile`
- `recvfile.c` (or `recvfile-fixed2.c`) → `recvfile`
- `terminal.c` → `terminal` (links `-pthread -lncurses -lform`)
  
Alternatively, build the two main tools manually:

```sh
cc -Wall -Wextra -O2 -c xfer.c -o xfer.o
cc -Wall -Wextra -O2 -c sendfile.c -o sendfile.o
cc -Wall -Wextra -O2 -o sendfile sendfile.o xfer.o

cc -Wall -Wextra -O2 -c recvfile.c -o recvfile.o
cc -Wall -Wextra -O2 -o recvfile recvfile.o xfer.o
```

> **Dependencies:** `terminal` requires `ncurses` and `form` (already part of base on FreeBSD).

---

## Usage

### 1) `sendfile` — send a text file to the Sharp

**Purpose:** Open the serial port and stream a text file to the PC‑G850 in SIO mode.  
**Line endings:** Each line is sent with `CRLF`.  
**EOF:** Appends CP/M style EOF byte `0x1A` at the end (expected by some Sharp tools).  
**Line numbers:** **No** numbering is inserted.

**Command line:**

```text
sendfile <serial-port> <filename>
```

**Examples:**

```sh
./sendfile /dev/cuaU0 program.bas
./sendfile /dev/ttyu0 mynotes.txt
```

**Behavior details:**
- Reads the input file line-by-line with a modest buffer (multiple reads handle long lines).
- Strips trailing `\r`/`\n` and then transmits the line followed by `\r\n`.
- After all data is sent, writes a single `0x1A` byte.
- Uses `init_fd()` from `xfer` for port setup.

---

### 2) `recvfile` — receive a text file from the Sharp

**Purpose:** Read bytes from the serial port and write a normalized text file on the host.  
**Line endings:** Any of `CRLF`, `CR`, or `LF` are normalized to `\n`.  
**EOF:** Stops if it encounters CP/M style EOF (`0x1A`).  
**Line numbers:** If a line **begins with digits** (optionally preceded by spaces), **a single space is inserted immediately after the digit sequence**. The digits themselves are preserved. No stripping is performed.

**Command line:**

```text
recvfile <serial-port> <output-filename>
```

**Examples:**

```sh
./recvfile /dev/cuaU0 program.bas
./recvfile /dev/ttyu0 capture.txt
```

**Behavior details:**
- Streams safely; handles partial reads and mixed line endings.
- Converts `CRLF`/`CR`/`LF` to `\n` in the output file.
- If a line starts with spaces + digits, the program ensures there is one space after the digits before any non‑digit character (useful for BASIC listings).
- Does **not** otherwise modify content.

---

### 3) `xfer.c` / `xfer.h` — serial setup helpers

**Purpose:** Provide a tiny API to open and configure the serial port for the PC‑G850 SIO link.

**Exported functions (declared in `xfer.h`):**

```c
int set_interface_attribs(int fd, int speed, int parity);
int set_blocking(int fd, int should_block);
int init_fd(char *port);
```

**Typical defaults:**
- 1200 baud, 8N1, blocking I/O.
- You can edit `init_fd()` in `xfer.c` if you need different speeds/parity.

**How to use:**
- `#include "xfer.h"` in your program.
- Link with `xfer.o` (do **not** `#include "xfer.c"` in sources).

---

### 4) `terminal` / `terminal.c`

**TBD**

---

## Notes & Tips

- **Serial device names:** On FreeBSD, USB serial adapters are often `/dev/cuaU0` (callout) or `/dev/ttyU0` (call‑in). The non‑modem *callout* device is usually what you want.
- **Flow control:** The helpers currently disable special processing and set raw-ish mode. If you need hardware flow control (RTS/CTS) or XON/XOFF, adjust `set_interface_attribs()`.
- **Baud rate:** The default is `B1200` to match common PC‑G850 SIO settings; change in `xfer.c` if necessary.
- **CP/M EOF (0x1A):** Commonly used by vintage systems to mark end-of-file. `recvfile` will stop when it sees it; `sendfile` appends one after transmit.
- **Line numbers:** `sendfile` does **not** insert them. `recvfile` only ensures a space after a leading numeric line label if present.

---

## License

MIT
