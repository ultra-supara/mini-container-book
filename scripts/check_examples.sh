#!/usr/bin/env bash
set -eu

portable_examples=(
  "01-basics/cmake-hello"
  "02-exec-result/print-string"
  "02-exec-result/file-io"
  "02-exec-result/stdio"
  "02-exec-result/execve"
  "02-exec-result/dup"
  "02-exec-result/pipe"
)

linux_examples=(
  "02-exec-result/clone"
  "02-exec-result/pid-namespace"
  "02-exec-result/network-namespace"
  "02-exec-result/user-namespace"
  "03-linux-security/e-sudo"
  "04-mini-container"
)

examples=("${portable_examples[@]}" "${linux_examples[@]}")

status=0

for example in "${examples[@]}"; do
  dir="examples/${example}"
  for file in README.md CMakeLists.txt src/main.c; do
    if [ ! -f "${dir}/${file}" ]; then
      printf 'missing %s\n' "${dir}/${file}" >&2
      status=1
    fi
  done
done

if [ "${status}" -ne 0 ]; then
  exit "${status}"
fi

if ! command -v cmake >/dev/null 2>&1; then
  printf 'example compile check skipped: cmake not found\n'
  exit 0
fi

build_examples=("${portable_examples[@]}")

if [ "$(uname -s)" = "Linux" ]; then
  build_examples+=("${linux_examples[@]}")
else
  printf 'Linux-only example compile check skipped on %s\n' "$(uname -s)"
fi

for example in "${build_examples[@]}"; do
  dir="examples/${example}"
  build_dir="${dir}/build"
  rm -rf "${build_dir}"
  cmake -S "${dir}" -B "${build_dir}" >/dev/null
  cmake --build "${build_dir}" >/dev/null
  if [ -x "${dir}/smoke_test.sh" ]; then
    "${dir}/smoke_test.sh"
  fi
done

printf 'checked %d example directories\n' "${#examples[@]}"
