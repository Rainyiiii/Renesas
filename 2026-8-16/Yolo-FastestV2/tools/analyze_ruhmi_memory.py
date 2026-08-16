from pathlib import Path
import argparse
import re


ARRAY_RE = re.compile(r"\b(?:u?int(?:8|16|32)_t|float|double)\s+\w+\s*\[\s*(\d+)\s*\]")
TYPE_SIZE = {"int8_t": 1, "uint8_t": 1, "int16_t": 2, "uint16_t": 2,
             "int32_t": 4, "uint32_t": 4, "float": 4, "double": 8}
BUFFER_RE = re.compile(r"kBufferSize_\w+\s*=\s*(\d+)")
ARENA_RE = re.compile(r"ARENA_SIZE\s+\(?\s*(\d+)")


def array_bytes(text: str) -> int:
    total = 0
    for type_name, count in re.findall(
        r"\b(u?int(?:8|16|32)_t|float|double)\s+\w+\s*\[\s*(\d+)\s*\]", text
    ):
        total += TYPE_SIZE[type_name] * int(count)
    return total


def main() -> None:
    parser = argparse.ArgumentParser(description="Estimate generated RUHMI static working memory.")
    parser.add_argument("src", type=Path,
                        help="conversion_results/.../build/MCU/compilation/src")
    args = parser.parse_args()

    model_c = args.src / "model.c"
    if not model_c.exists():
        raise SystemExit(f"Missing {model_c}")
    direct = array_bytes(model_c.read_text(encoding="utf-8", errors="ignore"))

    cpu = 0
    npu = 0
    for header in args.src.glob("*.h"):
        text = header.read_text(encoding="utf-8", errors="ignore")
        cpu += sum(map(int, BUFFER_RE.findall(text)))
        npu += sum(map(int, ARENA_RE.findall(text)))

    total = direct + cpu + npu
    print(f"model arrays : {direct:10d} bytes")
    print(f"CPU arenas   : {cpu:10d} bytes")
    print(f"NPU arenas   : {npu:10d} bytes")
    print(f"upper sum    : {total:10d} bytes ({total / 1024 / 1024:.2f} MiB)")
    print("Note: generated arenas may have reusable lifetimes; confirm with the final linker map.")


if __name__ == "__main__":
    main()
