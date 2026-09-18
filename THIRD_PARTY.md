# Third-party components

- **vrEmu6502**, Troy Schrapel, MIT. Pinned source, license, revision, and local modification notes are in `vendor/vrEmu6502/`.
- **Krusader 1.3/GAKMON**, Ken Wessen, Steve Wozniak, and Glenn Klaas. The user-requested source and matching linker configuration are in `vendor/krusader/`, with original attribution and a pinned revision. No additional license grant is asserted.
- **6502/65C02 functional tests**, Klaus Dormann. Pinned test binaries and decimal source in `tests/functional/`, with upstream GPL-3.0 license in `license.txt`. These fixtures are test data and are not linked into the emulator.
- **Decimal-mode test**, Bruce Clark. The source identifies itself as public domain. `tools/decimal_fixture.py` converts assembler syntax without changing the test algorithm and enables all checks.
- **OSI BASIC**, Microsoft 1977, with Grant Searle adaptations. Downloaded separately; see `roms/README.md`.
- **cc65 V2.19**, optional local build tool. `tools/bootstrap_cc65.py` downloads a checksum-pinned release into ignored `.tools/`, preserving its source and license. It is not linked into the emulator.
