#!/usr/bin/env bash

set -euo pipefail

parser_bin="${PARSER_BIN:-./webserv}"
verbose="${VERBOSE:-1}"
conf_dir="${CONF_DIR:-./conf}"
conf_files="${CONF_FILES:-}"
conf_filter="${CONF_FILTER:-*.conf}"
temp_unreadable=""
neg_failures=0

cleanup() {
  if [[ -n "$temp_unreadable" && -e "$temp_unreadable" ]]; then
    chmod 600 "$temp_unreadable" 2>/dev/null || :
    rm -f "$temp_unreadable" 2>/dev/null || :
  fi
}
trap cleanup EXIT

if [[ -z "$conf_files" ]]; then
  shopt -s nullglob
  files=("$conf_dir"/$conf_filter)
  shopt -u nullglob
else
  read -r -a files <<<"$conf_files"
fi

if [[ ${#files[@]} -eq 0 ]]; then
  echo "No configuration files found in '$conf_dir'." >&2
  exit 1
fi

echo -e "\n🔍 Running parser tests on config files..."
for f in "${files[@]}"; do
  echo "--------------------------------------"
  echo "Testing $f"
  if [[ "$verbose" == "1" ]]; then
    if "$parser_bin" "$f"; then
      echo "✅ $f -> OK"
    else
      echo "❌ $f -> FAILED"
    fi
  else
    if "$parser_bin" "$f" >/dev/null 2>&1; then
      echo "✅ $f -> OK"
    else
      echo "❌ $f -> FAILED"
    fi
  fi
done
echo "--------------------------------------"
echo "Test suite completed."

run_negative_test() {
  local label="$1"
  local path="$2"

  echo "--------------------------------------"
  echo "Negative test: $label"
  echo "Testing $path"

  if [[ "$verbose" == "1" ]]; then
    if "$parser_bin" "$path"; then
      echo "❌ $label -> Expected failure but the parser succeeded"
      neg_failures=1
    else
      echo "✅ $label -> Parser failed as expected"
    fi
  else
    if "$parser_bin" "$path" >/dev/null 2>&1; then
      echo "❌ $label -> Expected failure but the parser succeeded"
      neg_failures=1
    else
      echo "✅ $label -> Parser failed as expected"
    fi
  fi
}

echo "🔁 Running negative parser checks..."

missing_path="${conf_dir}/__nonexistent_config_$$.conf"
while [[ -e "$missing_path" ]]; do
  missing_path="${missing_path}_x"
done
#Loop ensures we really pick something unused (just in case).
run_negative_test "nonexistent file" "$missing_path"

run_negative_test "directory instead of file" "$conf_dir"

temp_unreadable=$(mktemp "${TMPDIR:-/tmp}/webserv_unreadable.XXXXXX")
printf "dummy" >"$temp_unreadable"
chmod 000 "$temp_unreadable"
run_negative_test "unreadable file" "$temp_unreadable"

echo "--------------------------------------"
echo "Negative checks completed."

exit "$neg_failures"
