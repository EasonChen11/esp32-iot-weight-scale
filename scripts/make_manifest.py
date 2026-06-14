#!/usr/bin/env python3
"""Generate manifest.json for a GitHub Release OTA update.

Usage:
    python scripts/make_manifest.py <version> [firmware.bin]

Writes manifest.json next to the firmware with the sha256 of the binary and the
permanent GitHub "latest" asset URL the device polls.
"""
import hashlib
import json
import sys
from pathlib import Path

REPO = "EasonChen11/esp32-iot-weight-scale"
ASSET_BASE = f"https://github.com/{REPO}/releases/latest/download"


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    version = sys.argv[1]
    bin_path = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(".pio/build/esp32dev/firmware.bin")
    if not bin_path.is_file():
        print(f"firmware not found: {bin_path}")
        return 1

    sha = hashlib.sha256(bin_path.read_bytes()).hexdigest()
    manifest = {
        "version": version,
        "url": f"{ASSET_BASE}/firmware.bin",
        "sha256": sha,
    }
    out = bin_path.parent / "manifest.json"
    out.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {out}\n{json.dumps(manifest, indent=2)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
