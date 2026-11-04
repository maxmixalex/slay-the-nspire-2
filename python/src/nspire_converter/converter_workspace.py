"""Owned converter workspace lifetimes and verified dependency boundaries.

Receipts survive reclamation. A descendant can reuse a verified result without
materializing its already-consumed ancestors. When a required boundary is absent
or has different inputs, its dependencies are rebuilt before it is run.
"""

from __future__ import annotations

import errno
import hashlib
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path
from typing import Any


def checksum(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def load_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (FileNotFoundError, ValueError):
        return {}


def scoped_identities(root, stages, external, inventory):
    """Bind all production sources and data; compiled bundles carry this inventory."""
    files = {
        key: value
        for key, value in inventory.items()
        if key.startswith(
            ("python/src/nspire_converter/", "python/dotnet/", "cpp/src/", "cpp/include/")
        )
        or key == "poetry.lock"
    }
    result = {}
    for stage in stages:
        inputs = {key: value for key, value in external.items() if not key.startswith("arm_")}
        if stage.name in ("pruned46", "final-index46", "validate-install"):
            inputs.update({key: value for key, value in external.items() if key.startswith("arm_")})
        result[stage.name] = checksum(dict(schema=2, source=files, inputs=inputs))
    return result


class SpaceMeter:
    """Sample logical work bytes and allocated bytes, labelling any estimate."""

    def __init__(self, work):
        self.work = Path(work)
        self.peak_bytes = self.peak_allocated_bytes = 0
        self.allocation_bytes_estimated = False
        self.samples = 0
        self.done = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def sample(self):
        size = allocated = 0
        seen = set()
        for folder, directories, files in os.walk(self.work, followlinks=False):
            directories[:] = [
                name for name in directories if not (Path(folder) / name).is_symlink()
            ]
            for name in files:
                try:
                    info = (Path(folder) / name).lstat()
                    size += info.st_size
                    inode = (info.st_dev, info.st_ino)
                    if not info.st_ino or inode not in seen:
                        blocks = getattr(info, "st_blocks", None)
                        if blocks is None:
                            blocks = (info.st_size + 511) // 512
                            self.allocation_bytes_estimated = True
                        allocated += blocks * 512
                        seen.add(inode)
                except FileNotFoundError:
                    pass
        self.peak_bytes = max(self.peak_bytes, size)
        self.peak_allocated_bytes = max(self.peak_allocated_bytes, allocated)
        self.samples += 1
        return size, allocated

    def _run(self):
        while not self.done.wait(0.5):
            self.sample()

    def start(self):
        self.sample()
        self.thread.start()

    def stop(self):
        self.done.set()
        self.thread.join()
        current, allocated = self.sample()
        return dict(
            peak_work_bytes=self.peak_bytes,
            peak_work_allocated_bytes=self.peak_allocated_bytes,
            retained_work_bytes=current,
            retained_work_allocated_bytes=allocated,
            measurement="recursive file-stat samples every 0.5 seconds and at stage boundaries",
            allocation_bytes_estimated=self.allocation_bytes_estimated,
            allocation_measurement=(
                "includes logical sizes rounded to 512 bytes where st_blocks is unavailable; filesystem allocation granularity not measured"
                if self.allocation_bytes_estimated
                else "filesystem st_blocks in 512-byte units"
            ),
            samples=self.samples,
        )


class WorkLock:
    """An OS-released lock also recovers after a killed converter process."""

    def __init__(self, work, *, blocking=False):
        self.work = Path(work)
        self.blocking = blocking

    def __enter__(self):
        self.work.mkdir(parents=True, exist_ok=True)
        self.stream = (self.work / ".converter-lock").open("a+b")
        try:
            # Reading the protected byte already raises EACCES on Windows.
            # Inspect metadata only, then let the OS lock arbitrate.
            if not os.fstat(self.stream.fileno()).st_size:
                try:
                    os.write(self.stream.fileno(), b"\0")
                except OSError:
                    # Another initializer may already own the now populated
                    # byte. Do not read it or flush a stale buffered write.
                    if not os.fstat(self.stream.fileno()).st_size:
                        raise
            self.stream.seek(0)
            if sys.platform == "win32":
                import msvcrt

                while True:
                    try:
                        msvcrt.locking(self.stream.fileno(), msvcrt.LK_NBLCK, 1)
                        break
                    except OSError as exc:
                        if not self.blocking or exc.errno not in (
                            errno.EACCES,
                            errno.EAGAIN,
                            errno.EDEADLK,
                        ):
                            raise
                        time.sleep(0.05)
            else:
                import fcntl

                fcntl.flock(
                    self.stream.fileno(), fcntl.LOCK_EX | (0 if self.blocking else fcntl.LOCK_NB)
                )
        except OSError as exc:
            self.stream.close()
            raise ValueError(
                "Another conversion is using this --work directory; choose a separate directory or wait for it to finish"
            ) from exc
        return self

    def __exit__(self, *args):
        if sys.platform == "win32":
            import msvcrt

            self.stream.seek(0)
            msvcrt.locking(self.stream.fileno(), msvcrt.LK_UNLCK, 1)
        self.stream.close()


class Workspace:
    # These small, expensive boundaries make profile changes and adding locales
    # practical without retaining every historical whole-pack generation.
    KEEP = frozenset(
        (
            "fonts",
            "scene-fonts",
            "ui15",
            "paired44",
            "cache/paired44",
            "retirement45",
            "cache/retirement45",
            "final-index46",
        )
    )
    SCRATCH_FLAGS = frozenset(("--cache", "--cache-dir", "--work", "--sample-work"))
    # These builtins take (input, output, work, ...), without a --work flag.
    # Other positional paths are inputs: notably the third pruning argument is
    # the linked ARM helper, which must never become disposable scratch.
    POSITIONAL_SCRATCH = {
        "@fresh-actor-dicing": 2,
        "@fresh-codec45": 2,
        "@final-quality": 2,
        "@final-sparse": 2,
        "@final-retune": 2,
    }

    def __init__(self, stages, work, identities, digest, atomic_json, *, keep=False):
        self.stages, self.work = stages, Path(work).resolve()
        self.identities, self.digest, self.atomic_json = identities, digest, atomic_json
        self.keep = keep
        self.receipts = self.work / "receipts"
        self.receipts.mkdir(parents=True, exist_ok=True)
        self.state_path = self.receipts / "workspace.json"
        old = load_json(self.state_path)
        self.owned = set(old.get("owned_roots", ())) if old.get("work") == str(self.work) else set()
        self.last_use, self.dependencies, self.writes = {}, [], []
        owners: dict[str | None, int] = {}
        for index, stage in enumerate(stages):
            paths = [Path(value) for value in stage.arguments if Path(value).is_absolute()]
            roots = {self.root(path) for path in paths} - {None}
            writes = {self.root(path) for path in stage.outputs} - {None}
            for at, flag in enumerate(stage.arguments[:-1]):
                if flag in self.SCRATCH_FLAGS:
                    scope = self.root(Path(stage.arguments[at + 1]))
                    if scope:
                        writes.add(scope)
            position = self.POSITIONAL_SCRATCH.get(stage.module)
            if position is not None:
                scope = self.root(Path(stage.arguments[position]))
                if scope:
                    writes.add(scope)
            dependencies: dict[int, set[str | None]] = {}
            for scope in roots:
                self.last_use[scope] = index
                if scope in owners:
                    dependencies.setdefault(owners[scope], set()).add(scope)
            self.dependencies.append(dependencies)
            self.writes.append(writes)
            for scope in writes:
                if not (self.work / scope).exists():
                    self.owned.add(scope)
                owners[scope] = index
                self.last_use[scope] = index
            if stage.name == "validate-install":
                parent = next(
                    i for i, prior in enumerate(stages[:index]) if prior.name == "publish-locales"
                )
                dependencies[parent] = {None}
        self.save_state()
        self.saved = [load_json(self.receipts / (stage.name + ".json")) for stage in stages]
        self.clean: list[bool] = []
        for index, stage in enumerate(stages):
            record = self.saved[index]
            self.clean.append(
                record.get("status") == "complete"
                and record.get("key") == self.key(index)
                and all(self.clean[parent] for parent in self.dependencies[index])
            )
        self.required = set()
        self.mark_needed(len(stages) - 1)
        self.reclaimed_bytes = 0

    def root(self, path):
        if not path.is_absolute():
            return None
        # Windows temporary paths can use RUNNER~1 while resolve() expands the
        # workspace to its long spelling. Canonicalize both sides consistently;
        # otherwise every dependency silently looks external to the workspace.
        path = path.resolve()
        try:
            parts = path.relative_to(self.work).parts
        except ValueError:
            return None
        if not parts or parts[0] in ("receipts", "logs"):
            return None
        return "/".join(parts[:2] if parts[0] == "cache" and len(parts) > 1 else parts[:1])

    def save_state(self):
        self.atomic_json(
            self.state_path, dict(schema=1, work=str(self.work), owned_roots=sorted(self.owned))
        )

    def key(self, index):
        stage = self.stages[index]
        inputs = {}
        for parent, roots in self.dependencies[index].items():
            record = self.saved[parent]
            inputs[self.stages[parent].name] = {
                name: value
                for name, value in record.get("outputs", {}).items()
                if self.root(Path(name)) in roots
            }
        identity = (
            self.identities[stage.name] if isinstance(self.identities, dict) else self.identities
        )
        # Frozen executables extract to a different directory on every launch.
        # Helper identities are in identity; normalize only the extraction root.
        from nspire_converter.paths import ROOT

        arguments = [value.replace(str(ROOT), "<recipe>") for value in stage.arguments]
        return checksum(
            dict(
                schema=2,
                recipe=identity,
                name=stage.name,
                module=stage.module,
                arguments=arguments,
                inputs=inputs,
            )
        )

    def verified(self, index, roots=()):
        record, stage = self.saved[index], self.stages[index]
        return (
            record.get("status") == "complete"
            and record.get("key") == self.key(index)
            and all(
                path.is_file() and self.digest(path) == record.get("outputs", {}).get(str(path))
                for path in stage.outputs
            )
            and all(scope is None or (self.work / scope).is_dir() for scope in roots)
        )

    def mark_needed(self, index, roots=()):
        if index < 0 or (self.clean[index] and self.verified(index, roots)):
            return
        if index in self.required:
            return
        self.required.add(index)
        for parent, required_roots in self.dependencies[index].items():
            self.mark_needed(parent, required_roots)

    def prepare_outputs(self, index):
        """Do not let an earlier all-language run leak stale locale inputs."""
        from nspire_converter.converter import LANGUAGES

        stage = self.stages[index]
        expected = set(stage.outputs)
        for directory in {path.parent for path in stage.outputs}:
            scope = self.root(directory)
            if scope not in self.owned:
                continue
            for code in LANGUAGES:
                for name in (f"nSpire2.{code}.dat.tns", f"nSpire2.{code}.tns"):
                    path = directory / name
                    if path not in expected and path.is_file():
                        path.unlink()

    def reclaim(self, index):
        if self.keep:
            return
        for scope in sorted(self.owned):
            if scope in self.KEEP or self.last_use.get(scope, len(self.stages)) > index:
                continue
            directory = self.work / scope
            if not directory.is_dir() or directory.is_symlink():
                continue
            # Refuse symlinked ancestors and descendants rather than following
            # anything that could be an owner-supplied external input.
            if self.work not in directory.resolve().parents:
                raise ValueError("Workspace cleanup escaped its owned directory")
            entries = list(directory.rglob("*"))
            if any(path.is_symlink() for path in entries):
                raise ValueError("Workspace cleanup refuses symbolic links: " + scope)
            size = sum(path.stat().st_size for path in entries if path.is_file())
            # Keep small diagnostic JSON outside the disposable generation.
            report_dir = self.receipts / "details" / scope
            for path in entries:
                if (
                    path.is_file()
                    and path.suffix == ".json"
                    and path.stat().st_size <= 2 * 1024 * 1024
                ):
                    target = report_dir / path.relative_to(directory)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(path, target)
            shutil.rmtree(directory)
            self.reclaimed_bytes += size


def run(stages, work, identities, stop_after=None, *, keep=False):
    with WorkLock(work):
        return _run(stages, work, identities, stop_after, keep=keep)


def _run(stages, work, identities, stop_after=None, *, keep=False):
    from nspire_converter import converter

    stages = list(stages)
    if stop_after:
        stages = stages[: next(i for i, stage in enumerate(stages) if stage.name == stop_after) + 1]
    workspace = Workspace(
        stages, work, identities, converter.digest, converter.atomic_json, keep=keep
    )
    logs = Path(work) / "logs"
    logs.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    meter = SpaceMeter(work)
    meter.start()
    summary: dict[str, Any] = dict(
        schema=1, complete=False, stages=[], started_at=time.time(), work=str(workspace.work)
    )
    try:
        for index, stage in enumerate(stages):
            if index not in workspace.required:
                print(
                    f"[{index + 1}/{len(stages)}] {stage.name}: verified downstream boundary",
                    flush=True,
                )
                summary["stages"].append(dict(stage=stage.name, action="boundary"))
                # A prior success can have stopped before reclamation, or have
                # used --keep-intermediates. The verified descendant already
                # consumed these roots; last_use still protects later readers.
                workspace.reclaim(index)
                meter.sample()
                continue
            receipt = workspace.receipts / (stage.name + ".json")
            key = workspace.key(index)
            if workspace.verified(index):
                print(
                    f"[{index + 1}/{len(stages)}] {stage.name}: verified existing output",
                    flush=True,
                )
                summary["stages"].append(dict(stage=stage.name, action="reused"))
            else:
                workspace.prepare_outputs(index)
                if stage.module == "@ui13-assets":
                    previous = [
                        parent
                        for parent in workspace.dependencies[index]
                        if stages[parent].module == "@ui13-assets"
                    ]
                    if previous:
                        snapshot = stages[max(previous)].outputs[0]
                        args = stage.arguments
                        shutil.copy2(
                            snapshot, Path(args[args.index("--cache") + 1]) / "ui13-assets.json"
                        )
                converter.prepare_tile_generation(
                    stage, work, receipt, logs / (stage.name + ".log")
                )
                print(
                    f"[{index + 1}/{len(stages)}] {stage.name}: converting (log: {logs / (stage.name + '.log')})",
                    flush=True,
                )
                began = time.monotonic()
                record = dict(stage=stage.name, key=key, status="running", started_at=time.time())
                converter.atomic_json(receipt, record)
                try:
                    with (logs / (stage.name + ".log")).open("w", encoding="utf-8") as output:
                        subprocess.run(
                            [
                                *converter.launch_prefix(),
                                "--internal-stage",
                                stage.module,
                                *stage.arguments,
                            ],
                            stdout=output,
                            stderr=subprocess.STDOUT,
                            check=True,
                        )
                    missing = [str(path) for path in stage.outputs if not path.is_file()]
                    if missing:
                        raise ValueError(
                            "Stage did not produce declared outputs: " + ", ".join(missing)
                        )
                    record.update(
                        status="complete",
                        elapsed_seconds=time.monotonic() - began,
                        outputs={str(path): converter.digest(path) for path in stage.outputs},
                    )
                    converter.atomic_json(receipt, record)
                    workspace.saved[index] = record
                    summary["stages"].append(
                        dict(
                            stage=stage.name,
                            action="converted",
                            elapsed_seconds=record["elapsed_seconds"],
                        )
                    )
                except BaseException as exc:
                    record.update(
                        status="failed", error=str(exc), elapsed_seconds=time.monotonic() - began
                    )
                    converter.atomic_json(receipt, record)
                    raise
            workspace.reclaim(index)
            meter.sample()
        summary["complete"] = not bool(stop_after)
        return not bool(stop_after)
    finally:
        summary.update(
            total_elapsed_seconds=time.monotonic() - started,
            reclaimed_bytes=workspace.reclaimed_bytes,
            **meter.stop(),
        )
        converter.atomic_json(workspace.receipts / "pipeline.json", summary)
        if summary["complete"]:
            allocation_label = (
                "estimated allocated bytes"
                if summary["allocation_bytes_estimated"]
                else "allocated bytes"
            )
            print(
                f"Workspace: {workspace.work}; retained {summary['retained_work_bytes']:,} logical bytes / "
                f"{summary['retained_work_allocated_bytes']:,} {allocation_label}.",
                flush=True,
            )
