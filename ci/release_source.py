#!/usr/bin/env python3
"""Find the current ISC BIND 9.20 ESV and authenticate its release tarball.

No third-party Python dependencies. GitHub Actions consumes GITHUB_OUTPUT.
Never derive a build SHA256 from an unverified upstream download.
"""

from __future__ import annotations

import hashlib
import html
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request

ISC_DOWNLOADS = "https://www.isc.org/download/"
ISC_KEYS = "https://www.isc.org/docs/isc-keyblock.asc"
ISC_BIND_BASE = "https://downloads.isc.org/isc/bind9"
VERSION_RE = re.compile(r"9\.20\.[0-9]+\Z")
OFFICIAL_SIGNERS = {
    # https://www.isc.org/pgpkey/ ; last checked 2026-10-30.
    "706B6C28620E76F91D11F7DF510A642A06C52CEC",
    "D99CCEAF879747014F038D63182E23579462EFAA",
    "0259A33B5F5A3A4466CF345C7A5E084CACA51884",
    "090A2A07923F925B5767803A42E5DF78C83271DB",
    "9580D6BF2CC80F1E3BB11252DEAB91D54B13C9B8",
    "FC874C3E3FE8677070AC71BEB5EFF6AC7E1ADDF8",
    "DA6A3508E672A49DD382AFD95B8F4D91B88ED909",
}


def download(url: str, destination: Path, limit: int = 40_000_000) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "static-dig-release-bot/1.0"})
    with urllib.request.urlopen(request, timeout=90) as response, destination.open("wb") as output:
        size = 0
        while chunk := response.read(1024 * 1024):
            size += len(chunk)
            if size > limit:
                raise ValueError(f"Download exceeds size limit: {url}")
            output.write(chunk)
    if not size:
        raise ValueError(f"Empty download: {url}")


def current_920_esv(document: str) -> str:
    """Only accept the explicit BIND 'Current Stable' + 'ESV' HTML table row."""
    candidates = []
    for row in re.findall(r"<tr\b[^>]*>(.*?)</tr>", document, flags=re.I | re.S):
        if not all(token in row for token in (
            "download-version-text", "badge-download--stable", "badge-download--esv", "BIND 9"
        )):
            continue
        version = re.search(
            r'class=["\']download-version-text["\'][^>]*>\s*(9\.20\.[0-9]+)\s*<',
            row, flags=re.I,
        )
        if version and "Current Stable" in html.unescape(re.sub(r"<[^>]+>", " ", row)):
            candidates.append(version.group(1))
    if len(candidates) != 1:
        raise ValueError(f"Expected exactly one stable BIND 9.20 ESV entry; got {candidates!r}")
    return candidates[0]


def already_released(version: str) -> bool:
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    if not repo:
        return False
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo):
        raise ValueError("Invalid GITHUB_REPOSITORY")
    url = f"https://api.github.com/repos/{repo}/releases/tags/v{version}"
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "static-dig-release-bot"}
    token = os.environ.get("GITHUB_TOKEN", "")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    try:
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=30):
            return True
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return False
        raise  # 403, rate limiting and API outages must never count as 'not released'.


def authenticated_sha256(version: str) -> str:
    archive = f"bind-{version}.tar.xz"
    with tempfile.TemporaryDirectory(prefix="static-dig-isc-") as temp:
        folder = Path(temp)
        src = folder / archive
        signature = folder / f"{archive}.asc"
        keys = folder / "isc-keyblock.asc"
        gnupg = folder / "gnupg"
        gnupg.mkdir(mode=0o700)
        download(f"{ISC_BIND_BASE}/{version}/{archive}", src)
        download(f"{ISC_BIND_BASE}/{version}/{archive}.asc", signature, limit=100_000)
        download(ISC_KEYS, keys, limit=1_000_000)

        command = ["gpg", "--homedir", str(gnupg), "--batch", "--no-tty"]
        subprocess.run(command + ["--import", str(keys)], check=True, capture_output=True, text=True)
        result = subprocess.run(
            command + ["--status-fd", "1", "--verify", str(signature), str(src)],
            check=False, capture_output=True, text=True,
        )
        if result.returncode:
            raise ValueError(f"ISC GPG signature verification failed: {result.stderr.strip()}")
        signers = set(re.findall(r"^\[GNUPG:\] VALIDSIG ([A-Fa-f0-9]{40})\b", result.stdout, re.M))
        if not signers or not (set(s.upper() for s in signers) & OFFICIAL_SIGNERS):
            raise ValueError(f"ISC source signature has no recognized signer: {signers!r}")
        with src.open("rb") as verified_source:
            digest = hashlib.file_digest(verified_source, "sha256").hexdigest()
        print(f"Verified ISC {archive} with trusted signing fingerprint(s): {', '.join(sorted(signers))}")
        return digest


def append_outputs(values: dict[str, str]) -> None:
    for key, value in values.items():
        print(f"{key}={value}")
    if output := os.environ.get("GITHUB_OUTPUT"):
        with open(output, "a", encoding="utf-8") as stream:
            for key, value in values.items():
                stream.write(f"{key}={value}\n")


def main() -> None:
    requested = os.environ.get("REQUESTED_VERSION", "").strip()
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    if requested:
        if not VERSION_RE.fullmatch(requested):
            raise ValueError("Only explicit 9.20.x versions are allowed")
        version = requested
    else:
        with tempfile.TemporaryDirectory(prefix="static-dig-page-") as temp:
            page = Path(temp) / "download.html"
            download(ISC_DOWNLOADS, page, limit=2_000_000)
            version = current_920_esv(page.read_text(encoding="utf-8"))

    exists = already_released(version)
    verify_only = event in {"pull_request", "push"}
    build_existing = os.environ.get("BUILD_EXISTING", "false").lower() == "true"
    if exists and not (verify_only or build_existing):
        print(f"v{version} already released. Nothing to do.")
        append_outputs({"version": version, "build": "false", "publish": "false", "source_sha256": ""})
        return

    digest = authenticated_sha256(version)
    # A branch/PR build is a test, never an automatic GitHub Release.
    publish = not exists and os.environ.get("GITHUB_REF", "refs/heads/main") == "refs/heads/main"
    publish = publish and event != "pull_request"
    append_outputs({
        "version": version,
        "build": "true",
        "publish": "true" if publish else "false",
        "source_sha256": digest,
    })


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"Release detection / source verification failed: {exc}", file=sys.stderr)
        sys.exit(1)
