#!/usr/bin/env python3
"""Run a command with a crash-readable journal written before it starts.

The supplied checkpoint is a receipt, not a remote verification: read the remote
branch first. This tool never pushes, enables CI, or treats an unfinished run as
success. Logs and journals stay under build/ unless an output directory is given.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[1]
INPUT_DIRS = ('src', 'include', 'platform', 'tools', 'tests', 'config', 'third_party')
INPUT_SUFFIXES = {'.cpp', '.hpp', '.h', '.inc', '.c', '.cs', '.S', '.ld', '.py', '.js', '.html', '.json', '.hex'}


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def inventory(root: Path) -> dict[str, str]:
    files = [root / n for n in ('CMakeLists.txt', 'requirements.txt',
                               'requirements-release.txt', 'VERSION')]
    for folder in INPUT_DIRS:
        files.extend(p for p in (root / folder).rglob('*') if p.suffix in INPUT_SUFFIXES)
    result = {}
    for path in sorted(set(files)):
        if path.is_symlink():
            raise ValueError(f'Symlink is not a checkpoint input: {path}')
        if path.is_file():
            result[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    return result


def atomic_json(path: Path, value: dict) -> None:
    temporary = path.with_suffix('.tmp')
    with temporary.open('w', encoding='utf-8') as stream:
        json.dump(value, stream, ensure_ascii=False, sort_keys=True, indent=2)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def stop(process: subprocess.Popen) -> None:
    if process.poll() is not None:
        return
    if os.name == 'posix':
        os.killpg(process.pid, signal.SIGTERM)
    else:
        process.terminate()
    try:
        process.wait(timeout=2)
    except subprocess.TimeoutExpired:
        if os.name == 'posix':
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        process.wait()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--checkpoint', required=True, help='Full commit SHA verified through the remote connector')
    parser.add_argument('--label', required=True)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--timeout', type=float, default=600)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not re.fullmatch(r'[0-9a-f]{40}', args.checkpoint):
        parser.error('--checkpoint must be a full 40-character commit SHA')
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,63}', args.label):
        parser.error('invalid --label')
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command or args.timeout <= 0:
        parser.error('provide a command and a positive timeout')
    root = args.root.resolve(strict=True)
    destination = args.output.resolve() if args.output else root / 'build' / 'journal'
    destination.mkdir(parents=True, exist_ok=True)
    token = f'{args.label}-{uuid.uuid4().hex[:12]}'
    journal, log = destination / (token + '.json'), destination / (token + '.log')
    inputs = inventory(root)
    report = {'schema': 1, 'checkpoint': args.checkpoint, 'command': command,
              'cwd': str(root), 'started_at': now(), 'status': 'running',
              'source_files': inputs, 'log': log.name,
              'source_sha256': hashlib.sha256(json.dumps(inputs, sort_keys=True).encode()).hexdigest()}
    atomic_json(journal, report)  # A forced stop leaves running, never passed.
    print(f'Journal: {journal}', flush=True)
    process = None
    code = 127
    try:
        with log.open('xb', buffering=0) as output:
            process = subprocess.Popen(command, cwd=root, stdout=output, stderr=subprocess.STDOUT,
                                       start_new_session=os.name == 'posix')
            report['pid'] = process.pid
            atomic_json(journal, report)
            code = process.wait(timeout=args.timeout)
            report['status'] = 'passed' if code == 0 else 'failed'
    except subprocess.TimeoutExpired:
        stop(process)
        code = 124
        report['status'] = 'timed_out'
    except KeyboardInterrupt:
        if process is not None:
            stop(process)
        code = 130
        report['status'] = 'interrupted'
    except OSError as error:
        if process is not None:
            stop(process)
        report['status'] = 'launch_error'
        report['error'] = str(error)
    finally:
        report['ended_at'] = now()
        report['returncode'] = code
        try:
            report['source_changed'] = inventory(root) != inputs
        except (OSError, ValueError):
            report['source_changed'] = True
        if report['source_changed'] and code == 0:
            report['status'] = 'source_changed'
            code = 1
        report['log_sha256'] = hashlib.sha256(log.read_bytes()).hexdigest() if log.exists() else None
        atomic_json(journal, report)
    print(f"{report['status']}: {journal}", flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
