#!/usr/bin/env bash
# Clone / refresh large regex corpora into test/pz-test/ (gitignored).
# Used by RegexSearch.LargeStressFiles via the PZTEST_DIR compile define.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="$ROOT/test/pz-test"
# Optional pin: PZTEST_PIN=<sha> ./scripts/setup_testdata.sh
PIN_COMMIT="${PZTEST_PIN:-HEAD}"

if [ ! -d "$TARGET/.git" ]; then
  git clone https://github.com/san-rizz-777/pz_test.git "$TARGET"
fi

git -C "$TARGET" fetch --quiet
git -C "$TARGET" checkout --quiet "$PIN_COMMIT"

# LargeStressFiles reads .txt (+ optional manifest.tsv) from here.
mkdir -p "$TARGET/regex/large"
echo "pz-test data ready at $TARGET"
echo "Large corpora: $TARGET/regex/large/ (optional manifest.tsv)"
