CC ?= cc
CA65 ?= $(if $(wildcard .tools/cc65-2.19/bin/ca65),.tools/cc65-2.19/bin/ca65,ca65)
LD65 ?= $(if $(wildcard .tools/cc65-2.19/bin/ld65),.tools/cc65-2.19/bin/ld65,ld65)
CPPFLAGS += -Isrc -Ivendor/vrEmu6502 -DVR_EMU_6502_STATIC
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
CORE = src/cpu.c src/acia.c src/machine.c src/config.c vendor/vrEmu6502/vrEmu6502.c
HEADERS = $(wildcard src/*.h) vendor/vrEmu6502/vrEmu6502.h
.PHONY: all clean krusader test sanitize fetch-basic
all: gk6502
gk6502: src/main.c $(CORE) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(CORE) $(LDFLAGS) -o $@
build:
	mkdir -p build
krusader: roms/krusader.bin
build/krusader.o: vendor/krusader/FULL_Krusader_1.3_ca65.asm | build
	$(CA65) $< -o $@ -l build/krusader.lst
roms/krusader.bin: build/krusader.o vendor/krusader/krusader_test.cfg
	$(LD65) -C vendor/krusader/krusader_test.cfg $< -o $@ -m build/krusader.map -Ln build/krusader.lbl
	python3 -c 'from pathlib import Path; assert Path("$@").stat().st_size == 16384'
fetch-basic:
	python3 tools/fetch_basic.py
build/test_core: tests/test_core.c $(CORE) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c $(CORE) $(LDFLAGS) -o $@
test: all krusader build/test_core
	./build/test_core
	python3 tests/integration.py
sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g -std=c11 -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' test
clean:
	rm -rf build gk6502
build/decimal_6502.s: tests/functional/6502_decimal_test.a65 tools/decimal_fixture.py | build
	python3 tools/decimal_fixture.py 0 > $@
build/decimal_65c02.s: tests/functional/6502_decimal_test.a65 tools/decimal_fixture.py | build
	python3 tools/decimal_fixture.py 1 > $@
build/decimal_%.o: build/decimal_%.s
	$(CA65) $< -o $@
build/decimal_%.bin: build/decimal_%.o tests/functional/decimal.cfg
	$(LD65) -C tests/functional/decimal.cfg $< -o $@
build/test_core: build/decimal_6502.bin build/decimal_65c02.bin

.PHONY: bootstrap-cc65
bootstrap-cc65:
	python3 tools/bootstrap_cc65.py
