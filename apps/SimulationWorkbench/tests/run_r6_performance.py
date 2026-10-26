#!/usr/bin/env python3
"""Opt-in, serial R6 performance gates. Build Release first; stop other workloads."""
import argparse
import hashlib
import json
import pathlib
import platform
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=pathlib.Path, default=pathlib.Path('cmake-build-release'))
parser.add_argument('--output', type=pathlib.Path, required=True, help='New evidence directory (never overwrites a run)')
args = parser.parse_args()
build = args.build.resolve()
cache = (build / 'CMakeCache.txt').read_text()
if 'CMAKE_BUILD_TYPE:STRING=Release\n' not in cache or 'PIPEFRAME_ANT_REWORK_ONLY:BOOL=ON\n' not in cache:
    parser.error('Use a Release build with PIPEFRAME_ANT_REWORK_ONLY=ON')
executable = build / 'apps/SimulationWorkbench/WorkbenchUITests'
args.output.mkdir(parents=True, exist_ok=False)
metadata = {'platform': platform.platform(), 'build': str(build),
            'executable_sha256': hashlib.sha256(executable.read_bytes()).hexdigest(), 'results': []}
fixtures = {'ant-scale': ['colonies-1.csv', 'colonies-3.csv', 'colonies-5.csv'],
            'ant-ui': ['editor-0.csv', 'editor-1.csv', 'editor-2.csv'],
            'ant-drag': ['editor-3.csv'], 'environment': ['map-384.csv', 'map-1024.csv']}
for repetition in range(1, 4):
    folder = args.output / f'run-{repetition}'
    folder.mkdir()
    for fixture, files in fixtures.items():
        command = [str(executable), f'--{fixture}-benchmark', str(folder.resolve())]
        with (folder / f'{fixture}.txt').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        valid_samples = all((folder / name).is_file() and len((folder / name).read_text().splitlines()) > 100 for name in files)
        metadata['results'].append({'run': repetition, 'fixture': fixture,
                                    'exit_code': result.returncode, 'samples_present': valid_samples})
        (args.output / 'results.json').write_text(json.dumps(metadata, indent=2) + '\n')
        print(f'{repetition}: {fixture}: exit {result.returncode}, samples {valid_samples}', flush=True)
sys.exit(0 if all(r['exit_code'] == 0 and r['samples_present'] for r in metadata['results']) else 1)
