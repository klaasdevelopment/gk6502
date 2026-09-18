#!/usr/bin/env python3
"""Translate Bruce Clark's public-domain AS65 fixture to ca65 syntax.
The test algorithm is unchanged; enable every flag and invalid-BCD case.
"""
import pathlib
import re
import sys
source = pathlib.Path('tests/functional/6502_decimal_test.a65').read_text()
model = int(sys.argv[1])
source = re.sub(r'^cputype = 0', f'cputype = {model}', source, flags=re.M)
for flag in ('n', 'v', 'z'):
    source = re.sub(rf'chk_{flag}\s*= 0', f'chk_{flag} = 1', source)
source = re.sub(r'end_of_test macro.*?endm', '', source, flags=re.S)
result = ['.setcpu "6502"']
for line in source.splitlines():
    code, sep, comment = line.partition(';')
    if not code.strip():
        result.append(line)
        continue
    if re.match(r'\s*bss\s*$', code): code = '.segment "ZEROPAGE"'
    elif re.match(r'\s*code\s*$', code): code = '.segment "CODE"'
    elif re.match(r'\s*(org|end)\b', code): continue
    elif re.match(r'\s*end_of_test\s*$', code): code = 'jmp DONE'
    else:
        code = code.replace('!=', '<>')
        code = re.sub(r'\bds\b', '.res', code)
        code = re.sub(r'^(\s*)(if|endif)\b', r'\1.\2', code)
        if re.match(r'^\w+', code) and '=' not in code:
            code = re.sub(r'^(\w+)', r'\1:', code)
    result.append(code + (sep + comment if sep else ''))
print('\n'.join(result))
