#!/usr/bin/env bash
# Print standalone function sizes from an ELF, and optionally turn a
# captured METRICS log into exec-time rows.
#
# Code size does not need a device. Cycle and instruction counts do.
# This script only reads a log the operator already captured.
#
# Usage:
#   scripts/release/collect_code_size.sh <soc> <elf>
#   scripts/release/collect_code_size.sh <soc> <elf> <metrics.log>
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <soc> <elf> [metrics.log]" >&2
    exit 2
fi

soc="$1"
elf="$2"
log="${3:-}"
repo="$(cd "$(dirname "$0")/../.." && pwd)"
nm="${NM:-riscv32-esp-elf-nm}"
sdkconfig="${repo}/src/soc/${soc}/include/sdkconfig.h"

if [[ ! -f "${elf}" ]]; then
    echo "ELF not found: ${elf}" >&2
    exit 1
fi

freq="$(sed -n 's/^#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ[[:space:]]*//p' "${sdkconfig}")"
if [[ -z "${freq}" ]]; then
    echo "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ missing in ${sdkconfig}" >&2
    exit 1
fi

echo "SoC ${soc}  CPU ${freq} MHz  ELF ${elf}"
echo
echo "function size_bytes"
"${nm}" --print-size --size-sort "${elf}" | while read -r _addr size _type name _; do
    case "${name}" in
        bist_*|wdt_init_windowed)
            printf '%s %d\n' "${name}" "$((16#${size}))"
            ;;
    esac
done

if [[ -z "${log}" ]]; then
    echo
    echo "No METRICS log. Cycle and instruction rows stay empty until a device capture is passed as the third argument."
    exit 0
fi

echo
echo "function mode delta exec_time"
# METRICS: name Mode: N delta=COUNT
while IFS= read -r line; do
    if [[ "${line}" =~ METRICS:\ ([^[:space:]]+)\ Mode:\ ([0-9]+)\ delta=([0-9]+) ]]; then
        name="${BASH_REMATCH[1]}"
        mode="${BASH_REMATCH[2]}"
        delta="${BASH_REMATCH[3]}"
        # exec_time_ms = cycles / (freq_MHz * 1000). Mode 0 is cycles.
        if [[ "${mode}" == "0" ]]; then
            exec="$(awk -v d="${delta}" -v f="${freq}" 'BEGIN { printf "%.3f ms", d / (f * 1000) }')"
        else
            exec="-"
        fi
        echo "${name} ${mode} ${delta} ${exec}"
    fi
done < "${log}"
