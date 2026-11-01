"""Explicit file-size context for host-only historical conversion stages.

These files are never loaded by the current calculator release. The final
paged format still validates its original directory, group, image and runtime
allocation bounds. Old standalone builders retain their historical limits.
"""
from contextlib import contextmanager
from contextvars import ContextVar

_HOST_INTERMEDIATES=ContextVar('nspire_host_intermediate_files',default=False)
HOST_FILE_LIMIT=256*1024*1024


def file_limit(historical_limit):
    return HOST_FILE_LIMIT if _HOST_INTERMEDIATES.get() else historical_limit


@contextmanager
def host_intermediate_files(enabled=True):
    token=_HOST_INTERMEDIATES.set(bool(enabled))
    try:
        yield
    finally:
        _HOST_INTERMEDIATES.reset(token)
