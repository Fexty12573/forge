#!/usr/bin/env python3
"""Apply the tracked injection fixes without changing dependency gitlinks."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PATCHES = [
    ('libs/iniparser', 'iniparser-injected.patch'),
    ('libs/libnx', 'libnx-injected.patch'),
    ('libs/nnheaders', 'nnheaders-nvn-alignment.patch'),
]


def apply(directory, patch, *options):
    return subprocess.run(
        ['git', '-C', str(ROOT / directory), 'apply', '--ignore-space-change',
         *options, str(ROOT / 'patches' / patch)],
        capture_output=True, text=True,
    )


def main():
    for directory, patch in PATCHES:
        if apply(directory, patch, '--reverse', '--check').returncode == 0:
            continue
        checked = apply(directory, patch, '--check')
        if checked.returncode:
            raise SystemExit(f'Cannot apply {patch} in {directory}. Initialize the pinned submodules '
                             f'and check for conflicting local changes.\n{checked.stderr}')
        result = apply(directory, patch)
        if result.returncode:
            raise SystemExit(result.stderr)
        print(f'applied ... {patch}')


if __name__ == '__main__':
    main()
