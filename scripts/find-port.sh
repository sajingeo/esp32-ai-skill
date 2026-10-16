#!/usr/bin/env bash
# find-port.sh [--all]
# Print the serial port of an attached ESP32-S3 (native USB-Serial-JTAG,
# Espressif VID 303a). With --all, list every candidate port.
# Exits 1 if none is found, 2 if several match (pass -p explicitly then).
set -uo pipefail

candidates=()
case "$(uname -s)" in
  Darwin)
    for p in /dev/cu.usbmodem*; do [[ -e "$p" ]] && candidates+=("$p"); done
    ;;
  Linux)
    # Prefer ports whose USB vendor is Espressif (0x303a).
    for p in /dev/serial/by-id/*; do
      [[ -e "$p" ]] || continue
      if [[ "$p" == *Espressif* || "$p" == *303a* ]]; then candidates+=("$(readlink -f "$p")"); fi
    done
    if [[ ${#candidates[@]} -eq 0 ]]; then
      for p in /dev/ttyACM*; do [[ -e "$p" ]] && candidates+=("$p"); done
    fi
    ;;
  MINGW*|MSYS*|CYGWIN*)
    echo "On Windows, find the COM port in Device Manager (Ports)." >&2
    exit 1
    ;;
esac

if [[ "${1:-}" == "--all" ]]; then
  printf '%s\n' "${candidates[@]:-}"
  exit 0
fi

case ${#candidates[@]} in
  0) echo "no ESP32-S3 port found: check the cable is a data cable, or enter download mode (hold BOOT while powering on)" >&2; exit 1 ;;
  1) echo "${candidates[0]}" ;;
  *) echo "several ports found, choose one with -p:" >&2; printf '  %s\n' "${candidates[@]}" >&2; exit 2 ;;
esac
