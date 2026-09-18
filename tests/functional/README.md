# Independent CPU fixtures

Upstream: https://github.com/Klaus2m5/6502_65C02_functional_tests
Pinned revision: `7954e2dbb49c469ea286070bf46cdd71aeb29e4b`.

- `6502_functional_test.bin`: full 64 KiB image, entry `$0400`, success loop `$3469`.
- `65C02_extended_opcodes_test.bin`: full 64 KiB image, entry `$0400`, success loop `$24F1`.
- `6502_decimal_test.a65`: Bruce Clark's public-domain decimal test, supplied by the same repository. `tools/decimal_fixture.py` translates AS65 directives to ca65, sets `cputype` to the selected model, enables N/V/Z flag checks in addition to A/C, includes invalid BCD inputs, and replaces the end marker with a self-loop. Code is linked at `$0200`; ERROR is zero-page `$0B`.

The runner uses a flat test bus, not the Searle memory map, and fails on unexpected self-loops, unsupported instructions, nonzero decimal ERROR, or an instruction limit. Board behavior is tested separately.

The upstream GPL-3.0 license is included as `license.txt`. The decimal source specifically declares itself public domain.
