from __future__ import annotations

import sys
import zipfile


def main() -> None:
    args = sys.argv[1:]
    if len(args) == 2 and args[0] == "-Z1":
        with zipfile.ZipFile(args[1]) as archive:
            sys.stdout.write("\n".join(archive.namelist()))
        return
    if len(args) == 3 and args[0] == "-p":
        with zipfile.ZipFile(args[1]) as archive:
            sys.stdout.buffer.write(archive.read(args[2]))
        return
    raise SystemExit("usage: unzip -Z1 archive | unzip -p archive entry")


if __name__ == "__main__":
    main()
