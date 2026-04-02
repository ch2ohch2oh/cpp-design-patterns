#!/usr/bin/env zsh
set -euo pipefail

clang_format="$(command -v clang-format || true)"
if [[ -z "${clang_format}" ]] && command -v xcrun >/dev/null 2>&1; then
  clang_format="$(xcrun --find clang-format 2>/dev/null || true)"
fi

if [[ -z "${clang_format}" ]]; then
  echo "clang-format not found. Install clang-format and re-run." >&2
  exit 1
fi

setopt extendedglob nullglob

files=( **/*.(c|cc|cpp|cxx|h|hh|hpp|hxx)(.N) )
files=( ${files:#build/**} )
files=( ${files:#.git/**} )

count="${#files[@]}"
if [[ "${count}" -eq 0 ]]; then
  echo "No source files found to format."
  exit 0
fi

for f in "${files[@]}"; do
  "${clang_format}" -i -- "${f}"
done
echo "Formatted ${count} files."
