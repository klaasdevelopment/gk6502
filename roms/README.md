# ROM provenance

## OSI BASIC

- Source: http://searle.x10host.com/6502/osi_bas.zip
- Specification: http://searle.x10host.com/6502/Simple6502.html
- Retrieved: 2026-09-17.
- Member: `osi_bas.bin`, 16,384 bytes, unmodified.
- SHA-256: `cc155a442a6a51c7f37f7f2e794fb2dcabdb21d44bd0e6c66e41d4584add478e`.
- Microsoft BASIC, copyright Microsoft 1977; serial adaptation by Grant Searle. Original rights retained. The download helper does not imply a new redistribution license.

## Krusader/GAKMON

- Source: https://github.com/glennklaas/6502/blob/867a4c5ccc66d5eccd28a15b173eb143be66b41e/krusader/FULL_Krusader_1.3_ca65.asm
- Linker configuration: `krusader/krusader_test.cfg` from the same revision.
- Source SHA-256: `9da6eb53ed49b2978e6d6ee4536b720be4d73435b31d6bdfdd3ee143b50dc061`.
- Built with ca65/ld65 V2.19, without source changes.
- Output: `krusader.bin`, 16,384 bytes.
- SHA-256: `21d7c256bcec97347af8adacb500cd6a80af0a2a9ddd2af0a749af973ecc07e2`.
- GAKMON `$C000–$C156`; Krusader `$F000–$FFC5`; I/O `$FFD2–$FFF6`; vectors `$FFFA–$FFFF`.
- All three vectors point to `$C000`. Krusader entry is `$F000`.
- Copyright Ken Wessen 2007, with Woz monitor and Glenn Klaas adaptations; attribution is preserved in the source.

Binary ROMs are local inputs/build artifacts and excluded from version control.
