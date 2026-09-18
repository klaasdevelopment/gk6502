# CPU dependency

Source: https://github.com/visrealm/vrEmu6502
Revision: `aae98cb14386d832cb7357c99626520b6590bc24` (also in `REVISION`).
Copyright Troy Schrapel; MIT license in `LICENSE`.

The C source is vendored with these local corrections, covered by the emulator tests:

- Allocate zeroed CPU state before the first reset; previously reset read uninitialized status/register state.
- Set I on both CPU reset variants and decrement SP by three; CMOS also clears D.
- Count seven cycles for IRQ/NMI entry and clear D on CMOS interrupt entry after saving status.
- Apply the NMOS JMP indirect page-wrap defect when reading the target pointer, not when fetching the instruction operand.
- Correct decimal ADC/SBC results and N/V/Z/C flags for both CPU variants, including invalid BCD operands. The complete Bruce Clark test passes with all flag checks enabled.

Original upstream `vrEmu6502.c` SHA-256 before these changes:
`148810a9477a003f1455e22dc50be7b2cc5312510816747eacae9cbdc67ae74f`.

`src/cpu.c` adapts callbacks to a machine context, selects NMOS or WDC behavior, reports unsupported NMOS opcodes, and keeps host controls responsive during WAI/STP.
