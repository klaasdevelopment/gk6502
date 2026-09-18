#!/usr/bin/env python3
"""Explicitly fetch the original, unmodified Grant Searle BASIC ROM."""
import hashlib
import io
from pathlib import Path
import urllib.request
import zipfile

URL = 'http://searle.x10host.com/6502/osi_bas.zip'
SHA256 = 'cc155a442a6a51c7f37f7f2e794fb2dcabdb21d44bd0e6c66e41d4584add478e'

def main():
    destination = Path(__file__).resolve().parents[1] / 'roms/osi_bas.bin'
    if destination.exists():
        if hashlib.sha256(destination.read_bytes()).hexdigest() == SHA256:
            print(f'Already verified: {destination}')
            return
        raise SystemExit(f'{destination} exists with a different checksum; move it aside before fetching')
    try:
        with urllib.request.urlopen(URL, timeout=30) as response:
            archive = zipfile.ZipFile(io.BytesIO(response.read()))
        data = archive.read('osi_bas.bin')
        if len(data) != 16384 or hashlib.sha256(data).hexdigest() != SHA256:
            raise ValueError('ROM size/checksum differs from the verified release')
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    except Exception as exc:
        raise SystemExit(f'BASIC download failed: {exc}\nDownload {URL} manually and extract osi_bas.bin into roms/.')
    print(f'Verified {destination}: {SHA256}')

if __name__ == '__main__': main()
