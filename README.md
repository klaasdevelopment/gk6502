# gk6502

A C11 terminal emulator for [Grant Searle’s minimal 6502 computer](http://searle.x10host.com/6502/Simple6502.html), with selectable NMOS 6502 and WDC 65C02 CPUs. It boots Grant’s unmodified OSI BASIC ROM and assembles and runs Glenn Klaas’s Krusader/GAKMON ROM.

## Build and run

Requires a POSIX system, a C compiler, Make, and Python 3.12+ for the tooling/tests. Building Krusader and the decimal tests also requires `ca65` and `ld65` from cc65.

```sh
make
make fetch-basic
./gk6502
```

If cc65 is not installed, build a pinned copy locally (requires internet access once):

```sh
make bootstrap-cc65
make krusader
./gk6502 --config krusader.properties
```

The bootstrap installs only into `.tools/`; the Makefile automatically finds those binaries. Alternatively set `CA65=/path/to/ca65 LD65=/path/to/ld65`. Krusader source is included at a pinned upstream revision and builds without source changes. Outputs are `roms/krusader.bin`, `build/krusader.map`, `build/krusader.lbl`, and `build/krusader.lst`.

`make fetch-basic` verifies an existing ROM or downloads `osi_bas.zip` from Grant’s site and extracts only `osi_bas.bin`. If the site is unavailable, manually extract that file to `roms/osi_bas.bin`. Downloads never occur during emulator startup or normal builds. See [ROM provenance](roms/README.md) for checksums.

## Properties

The default file is `emulator.properties`. ROM paths are relative to the properties file, regardless of the working directory.

```properties
# OSI BASIC
rom.file=roms/osi_bas.bin
cpu.model=6502
```

To select Krusader, change those values to:

```properties
rom.file=roms/krusader.bin
cpu.model=65c02
```

Or select the supplied `krusader.properties` using `--config`. Both properties are required. The parser accepts blank lines, leading/trailing whitespace, and full-line `#` comments. Values are literal (no quotes, escapes, environment expansion, or inline comments). Unknown/duplicate keys and invalid values fail with diagnostics. ROMs must be exactly 16,384 bytes and are mapped at `$C000`.

## Console and boot controls

| Key | Action |
| --- | --- |
| Enter | Send carriage return to the guest |
| Ctrl-C | Send BASIC’s break character |
| Ctrl-W | Warm boot: reset CPU and ACIA, preserve RAM |
| Ctrl-B | Cold boot: clear RAM, reset CPU and ACIA |
| Ctrl-] | Exit the emulator |

Both boots restart through the ROM reset vector and discard pending serial input. They retain the already loaded ROM and CPU model. Warm boot is a hardware reset; the ROM can subsequently alter RAM. For BASIC, choose `W` at its startup prompt to retain the BASIC program after Ctrl-W. Choose `C` after Ctrl-B. Ctrl-M is the same terminal byte as Enter and is therefore not a boot shortcut.

Terminal echo and canonical input are disabled while running and restored on normal exit and handled termination signals. Local echo should remain off. The ROM handles editing: OSI BASIC uses Delete (`$7F`); GAKMON uses Backspace (`$08`). The terminal must deliver the control keys to the process; terminal emulator shortcuts can intercept them first.

Control shortcuts also apply to redirected input and are consumed by the emulator immediately, so do not append Ctrl-] to a batch of commands that still needs to execute. CR, LF, and CRLF input are normalized to guest carriage returns. Scripted sessions should wait for guest prompts, as the acceptance tests do. EOF disconnects host input but leaves the guest running; use a cycle limit for unattended runs:

```sh
./gk6502 --config emulator.properties --max-cycles 10000000 < commands.txt > session.txt
```

Serial output goes to stdout; diagnostics go to stderr. Exit status is 0 for Ctrl-], 1 for errors, 2 for a cycle limit, or 128 plus the signal number for handled signals. The limit is cumulative across boots, includes reset cycles, and is checked between instructions.

### BASIC

On startup press `C`, then Enter at `MEMORY SIZE?` and `TERMINAL WIDTH?`. The ROM reports `32255 BYTES FREE` and `OK`.

```basic
PRINT 2+2
10 FOR I=1 TO 3
20 PRINT I
30 NEXT I
RUN
```

### Krusader

GAKMON starts at `$C000`. Enter `F000R` to start Krusader, then `N` to enter a new assembly program. A leading space advances past the label field. Enter these three lines:

```asm
 LDA #$2A
 STA $0400
 RTS
```

Press Escape to return to the `?` prompt. `L` lists the program, `A` assembles it at `$0300`, and `D $0300` disassembles it. **Press a key to stop the streaming disassembler** before entering another command. `R $0300` runs it. Ctrl-W returns to GAKMON; entering `0400` shows `2A`. Ctrl-B followed by `0400` shows `00`.

The supplied ROM has a small upstream banner-length defect that emits extra characters before “KRUSADER”. Its source is kept unchanged; this does not prevent editing, assembly, or execution. Krusader’s own assembler does not implement every WDC opcode even though the emulated CPU supports them.

## Hardware model

| Address range | Mapping |
| --- | --- |
| `$0000–$7FFF` | 32 KiB RAM, initially zero |
| `$8000–$9FFF` | Unmapped: reads `$FF`, writes ignored |
| `$A000–$BFFF` | MC6850 ACIA, repeated every two bytes |
| `$C000–$FFFF` | 16 KiB ROM, writes ignored |

Even ACIA addresses read status/write control; odd addresses read/write data. Searle’s schematic selects the ACIA using A15/A14/A13 and connects RS to A0. Its IRQ output controls host flow control through an inverter. CPU IRQ and NMI are tied inactive, so ACIA receive interrupts must **not** invoke CPU interrupt handlers.

The ACIA models a one-byte receive register, control/reset, RDRF, TDRE, receive/transmit IRQ status, and overrun clearing. A separate bounded host queue applies backpressure and supplies the next byte only after the previous byte is read. CTS/DCD are asserted, matching the schematic. Transmission is immediately ready; serial frames, parity generation, physical baud timing, and modem disconnections are not simulated.

The CPU counts instruction cycles (including page crossings and decimal-mode differences), but execution runs as fast as the host allows rather than at a fixed wall-clock MHz. Individual bus cycles, dummy accesses, NMOS undocumented opcodes, and electrical behavior are outside this functional model. Unsupported NMOS opcodes produce an address/opcode error. `65c02` selects WDC instructions, including bit operations, WAI, STP, and its defined NOP encodings. WAI/STP remain responsive to host reset controls.

CPU reset sets I and adjusts the stack pointer; CMOS reset also clears D. Warm reset preserves the other CPU registers. Register power-on values are deterministic emulator defaults, not claimed physical power-on values. CPU bus callbacks are serialized; this implementation is not a concurrent multi-machine API.

## Tests

```sh
make test
make sanitize
```

Tests cover:

- Klaus Dormann’s NMOS functional and 65C02 extended instruction suites.
- Bruce Clark’s exhaustive decimal test on both models: all 256×256 operands, both carry inputs, ADC/SBC, and accumulator/N/V/Z/C checks, including invalid BCD digits.
- Reset, IRQ entry/RTI, decimal flags, page-cross timing, indirect JMP differences, WAI/STP, memory boundaries, ACIA status/aliases, and RAM retention/clearing.
- Real BASIC and Krusader sessions, program execution, boot controls, Ctrl-C, properties/ROM errors, and pseudo-terminal restoration.

`make sanitize` performs a clean AddressSanitizer/UndefinedBehaviorSanitizer build and runs the same suite. Run `make clean && make` afterward for an optimized executable. The CPU dependency and local correctness fixes are described in [vendor/vrEmu6502/README.local.md](vendor/vrEmu6502/README.local.md). Test fixtures have their own upstream license; see [THIRD_PARTY.md](THIRD_PARTY.md).
