# Grant Searle emulator with selectable BASIC and Krusader ROMs

## Summary

Build a C11 terminal emulator for Searle’s memory map and MC6850 serial interface. Support the unmodified OSI BASIC ROM and a ROM assembled from the supplied Krusader source. Select the ROM and CPU model through an emulator properties file.

## Machine and CPU

- Separate CPU execution, memory bus, ACIA, configuration, and terminal handling.
- Implement selectable NMOS 6502 and 65C02 models, preserving their differences in decimal arithmetic, interrupt behavior, instruction timing, and indirect JMP behavior.
- Support documented instructions for both models, including 65C02 extensions. Unsupported opcodes stop with an address and opcode diagnostic.
- Map RAM at `$0000–$7FFF`, unmapped space at `$8000–$9FFF`, ACIA at `$A000–$BFFF`, and read-only ROM at `$C000–$FFFF`.
- Verify Searle’s schematic for register mirroring and interrupt wiring. Implement ACIA control/status, reset, receive/transmit readiness, and status clearing.
- Boot through the ROM reset vector. Default RAM to zero and unmapped reads to `$FF`.

## Properties and Console

- Use `./gk6502 --config PATH`, defaulting to `emulator.properties`.
- Define a plain UTF-8 `key=value` format with blank lines and `#` comments. Require `rom.file` and `cpu.model`; accept CPU values `6502` and `65c02`.
- Resolve relative ROM paths against the properties file’s directory. Reject missing, unknown, duplicate, or invalid properties with line-numbered diagnostics.
- Supply BASIC and Krusader example configurations:
  - BASIC: `rom.file=roms/osi_bas.bin`, `cpu.model=6502`.
  - Krusader: `rom.file=roms/krusader.bin`, `cpu.model=65c02`.
- Require a 16,384-byte ROM. ROM selection occurs only through properties.
- Support interactive input without local echo, Enter normalization to carriage return, Ctrl-C delivery to the guest, and Ctrl-] to exit. Restore terminal settings on exit.
- Reserve Ctrl-W (`$17`) for a warm boot: reset the CPU and ACIA, discard pending serial input, preserve RAM, and resume through the selected ROM’s reset vector.
- Reserve Ctrl-B (`$02`) for a cold boot: clear RAM to zero, reset the CPU and ACIA, discard pending serial input, and resume through the selected ROM’s reset vector, matching startup behavior.
- Consume boot shortcuts in the emulator rather than sending them to the guest. Keep the selected ROM and CPU model for both boots; do not reload properties or the ROM file. Apply shortcuts to interactive and redirected input.
- Support redirected serial input/output and `--max-cycles N` for bounded automation; send diagnostics to stderr.

## ROM Assembly and Build

- Provide a Makefile for the emulator and an explicit `make krusader` target using `ca65` and `ld65` from cc65.
- Assemble the exact [supplied source](https://github.com/glennklaas/6502/blob/master/krusader/FULL_Krusader_1.3_ca65.asm), preserving attribution and recording the upstream revision.
- Use the upstream `krusader_test.cfg` layout: monitor at `$C000`, Krusader at `$F000`, I/O routines at `$FFD2`, and vectors at `$FFFA`. Fill unused ROM bytes with `$FF` and produce the binary, map, and listing.
- Diagnose assembler errors or segment overflow explicitly. Keep any necessary source compatibility fixes minimal and documented.
- Acquire OSI BASIC from Searle’s archive without patches; record its source and SHA-256. Provide an explicit fetch helper and manual placement instructions.
- Document dependencies, build commands, properties, ROM switching, console controls, and sample sessions for both ROMs.

## Verification and Acceptance

- Run CPU functional and decimal tests for both models, including the relevant [Klaus Dormann suites](https://github.com/Klaus2m5/6502_65C02_functional_tests). Test interrupt semantics and instruction timing separately.
- Test memory boundaries, ROM write protection, ACIA aliases/status transitions, and configuration parsing/path resolution.
- Boot OSI BASIC through initialization; verify automatic RAM detection, arithmetic output, and a stored `FOR/NEXT` program.
- Assemble Krusader and verify its ROM size, segment layout, and reset vector. Boot to GAKMON, examine and modify RAM, enter Krusader using the linked entry address, then assemble, disassemble, and run a small program.
- Use prompt-aware serial tests with bounded execution and failure transcripts. Check terminal cleanup and invalid-ROM diagnostics.
- Test Ctrl-W and Ctrl-B with both ROM selections: verify reset-vector execution, ACIA reset, pending-input removal, shortcut consumption, RAM preservation on warm boot, and zero-filled RAM on cold boot before ROM initialization executes. Verify repeated boots and recovery from a running guest program.
- Provide `make test` and an optional sanitizer check. Report blocked ROM-dependent checks without claiming boot success.

## Assumptions

- Target POSIX systems with a C terminal CLI and selectable CPUs.
- Warm boot means hardware reset with RAM preserved, not a ROM-specific jump to an interpreter’s warm-start entry. ROM startup code may subsequently modify RAM; preservation does not guarantee that BASIC programs or Krusader editor state survive initialization.
- Fidelity covers functional hardware behavior and instruction cycle counts; individual bus cycles, physical serial timing, and NMOS undocumented opcodes are outside this version.
- Searle’s [original site](http://searle.x10host.com/6502/Simple6502.html) was inaccessible during planning but was retrieved during implementation. The schematic confirms ACIA mirroring via A0 and host flow control via ACIA IRQ; CPU IRQ/NMI are tied inactive. The original OSI ROM was retrieved and verified.

## Implementation Record

- Implemented the C terminal emulator, properties-based ROM/CPU selection, ACIA/memory model, Ctrl-W warm boot, and Ctrl-B cold boot. See `README.md` for build and console instructions.
- Vendored a pinned MIT-licensed CPU core with documented local correctness fixes. The `65c02` selection uses WDC behavior. CPU and ROM provenance are recorded alongside the sources.
- Assembled the supplied Krusader source unchanged with ca65/ld65 V2.19. Both ROMs boot and execute programs in automated console tests.
- Verified the NMOS and CMOS functional suites, exhaustive decimal arithmetic/flags, board/reset behavior, properties/error handling, and terminal cleanup. AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer checks passed outside the tracing sandbox.
