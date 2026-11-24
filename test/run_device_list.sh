#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -f "$tmp/device_list_request.inc" "$tmp/device-list-test"; rmdir "$tmp"' EXIT
sed -n '/^class N2kDeviceListRequest /,/^void handleConfigRequestData(/p' \
    src/main.cpp | sed '$d' > "$tmp/device_list_request.inc"
test -s "$tmp/device_list_request.inc"
g++ -std=c++11 -I"$tmp" -I.pio/libdeps/m5stack-atom/ArduinoJson/src \
    test/device_list.cpp -o "$tmp/device-list-test"
"$tmp/device-list-test"
