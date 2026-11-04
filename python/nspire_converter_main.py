"""Nuitka executable entry; all recipe modules are compiled package members."""

from nspire_converter.__main__ import main

if __name__ == "__main__":
    raise SystemExit(main())
