#!/usr/bin/env bash
# Regenerate cert/x509_crt_bundle.bin from the latest Mozilla CA root certificates.
#
# The release workflow runs this before building so every published firmware embeds
# an up-to-date root-CA set — no manual cert maintenance, nothing to remember.
# Roots are long-lived (years), but this keeps them fresh automatically and lets a
# future OTA ship updated roots without a USB trip.
#
# On ANY failure (download or generation) it keeps the committed bundle and exits 0,
# so a transient network hiccup never blocks a release.
#
# Requires: curl, python3 with the `cryptography` package (pip install cryptography).
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/cert/x509_crt_bundle.bin"
GEN="$ROOT/scripts/gen_crt_bundle.py"
CACERT_URL="https://curl.se/ca/cacert.pem"   # curl-maintained Mozilla CA bundle (PEM)

PYTHON="$(command -v python3 || command -v python || true)"
if [ -z "$PYTHON" ]; then
  echo "[ca-bundle] WARNING: no python found — keeping committed bundle"
  exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "[ca-bundle] downloading latest Mozilla roots from $CACERT_URL"
if ! curl -fsSL "$CACERT_URL" -o "$TMP/cacert.pem"; then
  echo "[ca-bundle] WARNING: download failed — keeping committed bundle"
  exit 0
fi

echo "[ca-bundle] generating bundle via gen_crt_bundle.py"
if ! ( cd "$TMP" && "$PYTHON" "$GEN" --input cacert.pem ); then
  echo "[ca-bundle] WARNING: generation failed — keeping committed bundle"
  exit 0
fi

cp "$TMP/x509_crt_bundle" "$OUT"
echo "[ca-bundle] updated $OUT ($(wc -c < "$OUT") bytes)"
