#!/usr/bin/env python3
"""ROM/console acceptance tests. No third-party Python packages required."""
import os
from pathlib import Path
import pty
import select
import signal
import subprocess
import tempfile
import termios
import time

ROOT = Path(__file__).resolve().parents[1]
EXE = str(ROOT / 'gk6502')

class Session:
    def __init__(self, config='emulator.properties', terminal=False):
        self.pending = b''
        self.transcript = b''
        self.master = self.slave = None
        args = [EXE, '--config', str(ROOT / config)]
        if terminal:
            self.master, self.slave = pty.openpty()
            self.saved = termios.tcgetattr(self.slave)
            self.p = subprocess.Popen(args, stdin=self.slave, stdout=self.slave, stderr=subprocess.PIPE)
            self.fd = self.master
        else:
            self.p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.fd = self.p.stdout.fileno()
    def send(self, data):
        if self.master is not None:
            os.write(self.master, data)
        else:
            self.p.stdin.write(data)
            self.p.stdin.flush()
    def expect(self, needle, timeout=5):
        deadline = time.monotonic() + timeout
        while needle not in self.pending:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise AssertionError(f'timeout waiting for {needle!r}\n{self.transcript[-6000:]!r}')
            if select.select([self.fd], [], [], remaining)[0]:
                data = os.read(self.fd, 4096)
                if not data:
                    raise AssertionError(f'guest exited {self.p.poll()}: {self.p.stderr.read()!r}\n{self.transcript[-6000:]!r}')
                self.pending += data
                self.transcript = (self.transcript + data)[-65536:]
        end = self.pending.index(needle) + len(needle)
        result, self.pending = self.pending[:end], self.pending[end:]
        return result
    def close(self):
        try:
            if self.p.poll() is None:
                self.send(b'\x1d')
                self.p.wait(timeout=3)
            assert self.p.returncode == 0, self.p.stderr.read()
            if self.slave is not None:
                assert termios.tcgetattr(self.slave) == self.saved, 'terminal not restored'
        finally:
            if self.p.poll() is None:
                self.p.kill()
                self.p.wait()
            if self.master is not None:
                os.close(self.master)
                os.close(self.slave)
    def __enter__(self): return self
    def __exit__(self, *_): self.close()


def basic_start(s):
    s.expect(b'Cold [C] or warm [W] start?')
    s.send(b'C')
    s.expect(b'MEMORY SIZE?')
    s.send(b'\r')
    s.expect(b'TERMINAL WIDTH?')
    s.send(b'\r')
    out = s.expect(b'OK\r\n')
    assert b'32255 BYTES FREE' in out
    assert b'OSI 6502 BASIC VERSION 1.0 REV 3.2' in out


def basic():
    with Session(terminal=True) as s:
        basic_start(s)
        s.send(b'PRINT 2+2\r')
        assert b' 4 ' in s.expect(b'OK\r\n')
        for line in (b'10 FOR I=1 TO 3', b'20 PRINT I', b'30 NEXT I'):
            s.send(line+b'\r')
            s.expect(line+b'\r\n')
        s.send(b'RUN\r')
        out = s.expect(b'OK\r\n')
        for number in (b' 1 ', b' 2 ', b' 3 '): assert number in out, out
        s.send(b'\x17garbage input to discard\r')
        s.expect(b'Cold [C] or warm [W] start?')
        s.send(b'W')
        s.expect(b'OK\r\n')
        s.send(b'LIST\r')
        assert b'10 FOR I=1 TO 3' in s.expect(b'OK\r\n')
        s.send(b'40 GOTO 40\r'); s.expect(b'40 GOTO 40\r\n')
        s.send(b'RUN\r'); s.expect(b' 3 ')
        s.send(b'\x03'); s.expect(b'OK\r\n')  # Ctrl-C reaches BASIC, not host SIGINT.
        s.send(b'RUN\r'); s.expect(b' 3 ')
        s.send(b'\x02ignored\r')
        basic_start(s)
        s.send(b'LIST\r')
        assert b'10 FOR' not in s.expect(b'OK\r\n')
        s.send(b'\x17')
        s.expect(b'Cold [C] or warm [W] start?')
    print('PASS BASIC boot, arithmetic, program, Ctrl-C, warm/cold boots, PTY cleanup')


def krusader():
    with Session('krusader.properties') as s:
        s.expect(b'WELCOME TO GAKMON V1.0')
        s.send(b'0400: 5A\r'); s.expect(b'0400:')
        s.send(b'\x17discard\r'); s.expect(b'WELCOME TO GAKMON V1.0')
        s.send(b'0400\r'); s.expect(b'0400: 5A')
        s.send(b'\x02discard\r'); s.expect(b'WELCOME TO GAKMON V1.0')
        s.send(b'0400\r'); s.expect(b'0400: 00')
        labels = (ROOT/'build/krusader.lbl').read_text().splitlines()
        entry = next(int(line.split()[1],16) for line in labels if line.endswith(' .MAIN'))
        assert entry == 0xf000
        s.send(f'{entry:04X}R\r'.encode()); s.expect(b'KRUSADER 65C02 BY KEN WESSEN 1.3'); s.expect(b'? ')
        s.send(b'N\r'); s.expect(b'000 ')
        for index, line in enumerate((b' LDA #$2A', b' STA $0400', b' RTS')):
            s.send(line+b'\r'); s.expect(f'{index+1:03X} '.encode())
        s.send(b'\x1b'); s.expect(b'? ')
        s.send(b'L\r'); out=s.expect(b'? ')
        assert b'LDA #$2A' in out and b'STA $0400' in out
        s.send(b'A\r'); assert b'0300-0305' in s.expect(b'? ')
        s.send(b'D $0300\r'); out=s.expect(b'RTS')
        assert b'A9 2A' in out and b'8D 00 04' in out
        s.send(b' '); s.expect(b'? ')  # Disassembly streams until any key.
        s.send(b'R $0300\r'); s.expect(b'? ')
        s.send(b'\x17'); s.expect(b'WELCOME TO GAKMON V1.0')
        s.send(b'0400\r'); s.expect(b'0400: 2A')
        s.send(b'0500: 4C 00 05\r'); s.expect(b'0500:')
        s.send(b'0500R\r'); s.expect(b'0500: 4C')
        s.send(b'\x02'); s.expect(b'WELCOME TO GAKMON V1.0')
        s.send(b'0400\r'); s.expect(b'0400: 00')
    print('PASS Krusader assembly, monitor, editor, assembler, disassembler, execution, boots')


def config_tests():
    with tempfile.TemporaryDirectory() as directory:
        d=Path(directory)
        (d/'rom.bin').write_bytes((ROOT/'roms/osi_bas.bin').read_bytes())
        cfg=d/'test.properties'
        def run(text, expected, code=1):
            cfg.write_text(text)
            p=subprocess.run([EXE,'--config',str(cfg),'--max-cycles','10000'],
                             cwd='/tmp',input=b'',capture_output=True,timeout=3)
            assert p.returncode==code, (p.returncode,p.stderr)
            assert expected in p.stderr, p.stderr
            return p
        valid='rom.file=rom.bin\ncpu.model=6502\n'
        p=run('# comment\n\n'+valid,b'cycle limit reached',2)
        assert b'Cold [C]' in p.stdout
        for text,message in [('rom.file=rom.bin\n',b'missing'),
                             (valid+'cpu.model=6502\n',b'duplicate'),
                             (valid+'typo=1\n',b'unknown'),
                             (valid.replace('6502','bad'),b'cpu.model must'),
                             ('rom.file\n',b'expected key=value'),
                             (valid.replace('rom.bin',''),b'empty'),
                             (valid.replace('rom.bin','absent'),b'cannot open ROM')]:
            run(text,message)
        for size in (0,16383,16385):
            (d/'rom.bin').write_bytes(bytes(size));run(valid,b'exactly 16384')
        image=bytearray(16384);image[0]=2;image[-4:]=bytes([0,0xc0,0,0xc0])
        (d/'rom.bin').write_bytes(image);run(valid,b'unsupported opcode $02 at $C000')
    s=Session(terminal=True)
    try:
        s.expect(b'Cold [C]')
        s.p.send_signal(signal.SIGTERM);s.p.wait(timeout=3)
        assert s.p.returncode==128+signal.SIGTERM
        assert termios.tcgetattr(s.slave)==s.saved
    finally:
        if s.p.poll() is None: s.p.kill();s.p.wait()
        os.close(s.master);os.close(s.slave)
    print('PASS properties, relative paths, ROM errors, opcode errors, signal cleanup')


if __name__=='__main__':
    basic()
    krusader()
    config_tests()
