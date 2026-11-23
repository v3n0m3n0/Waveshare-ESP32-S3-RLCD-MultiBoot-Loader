#!/usr/bin/env python3
"""
Publishes the v1.0.0 GitHub release and uploads all release assets.
Uses git credentials to obtain the GitHub token.
"""
import os
import subprocess
import json
import urllib.request
import urllib.error
from pathlib import Path

def get_git_token():
    p = subprocess.Popen(
        ["git", "credential", "fill"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    out, _ = p.communicate(input="protocol=https\nhost=github.com\n")
    for line in out.splitlines():
        if line.startswith("password="):
            return line[9:]
    raise RuntimeError("Could not retrieve GitHub token from git credentials")

def main():
    root = Path(__file__).resolve().parent.parent
    release_dir = root / "release"
    notes_file = root / "docs" / "releases" / "v1.0.0.md"
    
    if not notes_file.exists():
        notes_file = root / "RELEASE_NOTES.md"
    
    body = notes_file.read_text(encoding="utf-8")
    token = get_git_token()
    
    owner = "v3n0m3n0"
    repo = "Waveshare-ESP32-S3-RLCD-MultiBoot-Loader"
    tag = "v1.0.0"
    release_name = "Waveshare ESP32-S3 RLCD MultiBoot Loader v1.0.0"
    
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "ReleasePublisher/1.0"
    }
    
    # 1. Check if release already exists
    req = urllib.request.Request(
        f"https://api.github.com/repos/{owner}/{repo}/releases/tags/{tag}",
        headers=headers
    )
    release = None
    try:
        with urllib.request.urlopen(req) as resp:
            release = json.loads(resp.read().decode("utf-8"))
            print(f"Found existing release ID {release['id']}")
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print("Release does not exist yet. Creating...")
        else:
            raise
    
    # 2. Create release if not exists
    if not release:
        payload = json.dumps({
            "tag_name": tag,
            "target_commitish": "main",
            "name": release_name,
            "body": body,
            "draft": False,
            "prerelease": False
        }).encode("utf-8")
        
        create_req = urllib.request.Request(
            f"https://api.github.com/repos/{owner}/{repo}/releases",
            data=payload,
            headers={**headers, "Content-Type": "application/json"},
            method="POST"
        )
        with urllib.request.urlopen(create_req) as resp:
            release = json.loads(resp.read().decode("utf-8"))
            print(f"Created release ID {release['id']}: {release['html_url']}")
            
    release_id = release["id"]
    upload_url_base = f"https://uploads.github.com/repos/{owner}/{repo}/releases/{release_id}/assets"
    
    # Get existing assets
    existing_assets = {a["name"]: a["id"] for a in release.get("assets", [])}
    
    # Files to upload
    files_to_upload = [
        "waveshare-esp32s3-rlcd-multiboot-loader-full-16MB-v1.0.0.bin",
        "rlcd-launcher-v1.0.0.bin",
        "bootloader.bin",
        "partitions.bin",
        "otadata.bin",
        "flash.ps1",
        "flash.bat",
        "SHA256SUMS.txt"
    ]
    
    for filename in files_to_upload:
        filepath = release_dir / filename
        if not filepath.exists():
            print(f"WARNING: File {filename} not found in release dir, skipping.")
            continue
            
        if filename in existing_assets:
            print(f"Asset {filename} already exists (ID: {existing_assets[filename]}), deleting to re-upload...")
            del_req = urllib.request.Request(
                f"https://api.github.com/repos/{owner}/{repo}/releases/assets/{existing_assets[filename]}",
                headers=headers,
                method="DELETE"
            )
            with urllib.request.urlopen(del_req) as resp:
                print(f"Deleted old {filename}")
                
        content = filepath.read_bytes()
        upload_url = f"{upload_url_base}?name={filename}"
        
        content_type = "application/octet-stream"
        if filename.endswith(".txt") or filename.endswith(".ps1") or filename.endswith(".bat"):
            content_type = "text/plain"
            
        up_req = urllib.request.Request(
            upload_url,
            data=content,
            headers={
                **headers,
                "Content-Type": content_type,
                "Content-Length": str(len(content))
            },
            method="POST"
        )
        print(f"Uploading {filename} ({len(content)} bytes)...")
        with urllib.request.urlopen(up_req) as resp:
            asset_info = json.loads(resp.read().decode("utf-8"))
            print(f"Uploaded {filename} -> ID {asset_info['id']}")

    print("\nRelease publishing completed successfully!")
    print(f"Release URL: {release['html_url']}")

if __name__ == "__main__":
    main()
