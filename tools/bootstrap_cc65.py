#!/usr/bin/env python3
"""Install a pinned ca65/ld65 toolchain inside .tools; no system changes."""
import hashlib
import io
from pathlib import Path
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
URL = 'https://codeload.github.com/cc65/cc65/tar.gz/refs/tags/V2.19'
SHA256 = '157b8051aed7f534e5093471e734e7a95e509c577324099c3c81324ed9d0de77'

def main():
    tools = ROOT / '.tools'
    tools.mkdir(exist_ok=True)
    cache = tools / 'cc65-2.19.tar.gz'
    if not cache.exists():
        with urllib.request.urlopen(URL, timeout=60) as response:
            data = response.read()
    else:
        data = cache.read_bytes()
    if hashlib.sha256(data).hexdigest() != SHA256:
        raise SystemExit('cc65 source checksum mismatch')
    cache.write_bytes(data)
    source = tools / 'cc65-2.19'
    if not source.exists():
        with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
            archive.extractall(tools, filter='data')
    subprocess.run(['make', '-C', str(source), '-j2', 'ca65', 'ld65'], check=True)
    print(f'Local assembler/linker ready in {source / "bin"}')

if __name__ == '__main__': main()
