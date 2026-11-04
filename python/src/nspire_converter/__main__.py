"""Console and Nuitka entry point."""

from nspire_converter.converter_threads import configure

configure()


def main() -> int:
    import multiprocessing

    multiprocessing.freeze_support()
    from nspire_converter.converter import main as run

    return int(run())


if __name__ == "__main__":
    raise SystemExit(main())
