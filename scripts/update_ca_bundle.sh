#!/usr/bin/env bash
# Regenerate cert/x509_crt_bundle.bin from the latest Mozilla CA root certificates.
#
# The release workflow runs this before building so every published firmware embeds
# an up-to-date root-CA set — no manual cert maintenance, nothing to remember.
# Roots are long-lived (years), but this keeps them fresh automatically and lets a
# future OTA ship updated roots without a USB trip.
#
# On ANY failure (download or generation) it keeps the committed bundle and exits 0,
# so a transient network hiccup never blocks a release. BUT it does NOT pass silently:
# it raises a GitHub Actions ::warning:: annotation and writes a visible block to the
# run summary, so a persistent "can't fetch certs but releases keep passing" situation
# is surfaced instead of hidden.
#
# Requires: curl, python3 with the `cryptography` package (pip install cryptography).
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/cert/x509_crt_bundle.bin"
GEN="$ROOT/scripts/gen_crt_bundle.py"
CACERT_URL="https://curl.se/ca/cacert.pem"   # curl-maintained Mozilla CA bundle (PEM)

# Surface a non-blocking failure: GitHub annotation + run-summary block, but exit 0
# so the release still publishes with the committed bundle.
warn_and_keep() {
  local msg="$1"
  echo "::warning title=CA bundle not refreshed::$msg — released firmware uses the committed (possibly stale) cert/x509_crt_bundle.bin"
  if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
      echo "### ⚠️ CA bundle was NOT refreshed"
      echo ""
      echo "$msg"
      echo ""
      echo "The release was published using the **committed** \`cert/x509_crt_bundle.bin\`, which may be out of date. The release still succeeded — but if this keeps happening, the embedded root CAs are going stale. Investigate the failure above."
    } >> "$GITHUB_STEP_SUMMARY"
  fi
  exit 0
}

PYTHON="$(command -v python3 || command -v python || true)"
if [ -z "$PYTHON" ]; then
  warn_and_keep "no python interpreter found to run gen_crt_bundle.py"
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "[ca-bundle] downloading latest Mozilla roots from $CACERT_URL"
if ! curl -fsSL "$CACERT_URL" -o "$TMP/cacert.pem"; then
  warn_and_keep "download from $CACERT_URL failed"
fi

echo "[ca-bundle] generating bundle via gen_crt_bundle.py"
if ! ( cd "$TMP" && "$PYTHON" "$GEN" --input cacert.pem ); then
  warn_and_keep "gen_crt_bundle.py failed to parse the downloaded PEM"
fi

cp "$TMP/x509_crt_bundle" "$OUT"
echo "[ca-bundle] updated $OUT ($(wc -c < "$OUT") bytes)"
