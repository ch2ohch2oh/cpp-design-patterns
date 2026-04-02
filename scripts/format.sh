#!/usr/bin/env bash
set -euo pipefail

clang_format=""
if command -v clang-format >/dev/null 2>&1; then
  clang_format="clang-format"
elif command -v xcrun >/dev/null 2>&1 && xcrun --find clang-format >/dev/null 2>&1; then
  clang_format="$(xcrun --find clang-format)"
fi

if [[ -z "${clang_format}" ]]; then
  echo "clang-format not found. Install clang-format and re-run." >&2
  exit 1
fi

if git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' 2>/dev/null | head -n 1 | grep -q .; then
  count="$(git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' | wc -l | tr -d ' ')"
  git ls-files -z -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' | xargs -0 "${clang_format}" -i
  echo "Formatted ${count} files (tracked)."
  exit 0
fi

echo "No tracked files yet; formatting all matching source files in the repo."
count="$(find . -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) -not -path './build/*' -not -path './.git/*' | wc -l | tr -d ' ')"
if [[ "${count}" == "0" ]]; then
  echo "No source files found to format."
  exit 0
fi

find . -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) -not -path './build/*' -not -path './.git/*' -print0 | xargs -0 "${clang_format}" -i
echo "Formatted ${count} files."
