import os
import pathlib
import sys
import subprocess
import hashlib
import struct
import zlib

def main():
    repo_dir = pathlib.Path(__file__).resolve().parent.parent
    os.chdir(repo_dir)
    print(f"[*] Repository directory: {repo_dir}")

    ver_file = repo_dir / "version.txt"
    if not ver_file.exists():
        sys.exit("Error: version.txt not found")
    ver = ver_file.read_text(encoding="utf-8").strip()
    print(f"[*] Packaging Release v{ver}")

    build_dir = repo_dir / ".pio" / "build" / "esp32-s3-rlcd-launcher"
    release_dir = repo_dir / "release"
    release_dir.mkdir(exist_ok=True)

    bootloader = build_dir / "bootloader.bin"
    partitions = build_dir / "partitions.bin"
    firmware = build_dir / "firmware.bin"

    # 1. Compile with PlatformIO if binaries do not exist or --clean requested
    if "--clean" in sys.argv or not (bootloader.exists() and partitions.exists() and firmware.exists()):
        pio_exe = r"C:\Users\KB\.platformio\penv\Scripts\pio.exe"
        print("[*] Running PlatformIO compilation...")
        cmd = [pio_exe, "run", "-e", "esp32-s3-rlcd-launcher"]
        subprocess.run(cmd, check=True)

    if not (bootloader.exists() and partitions.exists() and firmware.exists()):
        sys.exit("Error: Build artifacts missing in .pio/build/")

    # 2. Generate standard otadata.bin (8192 bytes, initial boot partition app0)
    otadata_path = release_dir / "otadata.bin"
    ota_seq = 1
    entry = struct.pack("<I20sII", ota_seq, b"", 0, 0)
    crc = zlib.crc32(entry[:28]) & 0xFFFFFFFF
    entry = struct.pack("<I20sII", ota_seq, b"", 0, crc)
    sector0 = entry + b"\xFF" * (4096 - len(entry))
    sector1 = b"\xFF" * 4096
    otadata_path.write_bytes(sector0 + sector1)
    print(f"[*] Generated otadata.bin (initial boot: app0, {len(sector0 + sector1)} B)")

    # 3. Copy binaries to release/
    dest_boot = release_dir / "bootloader.bin"
    dest_part = release_dir / "partitions.bin"
    dest_app = release_dir / f"rlcd-launcher-v{ver}.bin"
    dest_boot.write_bytes(bootloader.read_bytes())
    dest_part.write_bytes(partitions.read_bytes())
    dest_app.write_bytes(firmware.read_bytes())
    print(f"[*] Copied bootloader, partitions, and {dest_app.name}")

    # 4. Merge full 16MB 0x0 factory image
    dest_full = release_dir / f"waveshare-esp32s3-rlcd-multiboot-loader-full-16MB-v{ver}.bin"
    esptool_py = pathlib.Path(r"C:\Users\KB\.platformio\packages\tool-esptoolpy\esptool.py")
    python_exe = sys.executable

    merged_ok = False
    if esptool_py.exists():
        print(f"[*] Merging 16 MB factory image via {esptool_py.name} ...")
        merge_cmd = [
            python_exe, str(esptool_py), "--chip", "esp32s3", "merge_bin",
            "-o", str(dest_full),
            "--flash_mode", "qio",
            "--flash_size", "16MB",
            "--flash_freq", "80m",
            "0x0", str(dest_boot),
            "0x8000", str(dest_part),
            "0xe000", str(otadata_path),
            "0x10000", str(dest_app)
        ]
        res = subprocess.run(merge_cmd)
        if res.returncode == 0:
            merged_ok = True

    if not merged_ok:
        print("[*] Merging 16 MB factory image via native Python fallback...")
        full_size = 16 * 1024 * 1024
        buf = bytearray(b"\xFF" * full_size)
        segments = [
            (0x0, dest_boot.read_bytes()),
            (0x8000, dest_part.read_bytes()),
            (0xE000, otadata_path.read_bytes()),
            (0x10000, dest_app.read_bytes()),
        ]
        for offset, data in segments:
            buf[offset : offset + len(data)] = data
        dest_full.write_bytes(buf)
        merged_ok = True

    print(f"[OK] Factory image created: {dest_full.name} ({dest_full.stat().st_size} bytes)")

    # 5. Generate SHA256SUMS.txt
    print("[*] Generating SHA256SUMS.txt ...")
    manifest_lines = []
    for item in sorted(release_dir.iterdir()):
        if item.is_file() and item.name != "SHA256SUMS.txt":
            sha = hashlib.sha256(item.read_bytes()).hexdigest().lower()
            manifest_lines.append(f"{sha}  {item.name}")
            print(f"  {sha[:16]}...  {item.name}")

    (release_dir / "SHA256SUMS.txt").write_text("\n".join(manifest_lines) + "\n", encoding="utf-8")
    print("[OK] Release packaging complete!")

if __name__ == "__main__":
    main()
