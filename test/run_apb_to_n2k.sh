#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -f "$tmp/convert_apb.inc" "$tmp/apb-test"; rmdir "$tmp"' EXIT
sed -n '/^    double normalizeBearing(/,/^    void convertRMC(/p' \
    lib/nmea0183ton2k/NMEA0183DataToN2K.cpp | sed '$d' > "$tmp/convert_apb.inc"
test -s "$tmp/convert_apb.inc"
nmea=../NMEA0183
n2k=.pio/libdeps/m5stack-atom/ttlappalainen_NMEA2000/src
g++ -std=c++11 -ffunction-sections -fdata-sections -I"$tmp" -I"$nmea" -I"$n2k" \
    test/apb_to_n2k.cpp "$nmea/NMEA0183Msg.cpp" "$nmea/NMEA0183Messages.cpp" \
    "$n2k/N2kMsg.cpp" "$n2k/N2kMessages.cpp" -Wl,--gc-sections -o "$tmp/apb-test"
"$tmp/apb-test"
