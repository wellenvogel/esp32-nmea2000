# Special conversion details

## NMEA 0183 APB to NMEA 2000

APB is converted to PGN 129283 (cross-track error) and PGN 129284 (navigation
data). PGN 129284 uses the current shared distance and waypoint coordinates,
when available; APB itself does not identify their source or verify that they
belong to its destination. Check that the input sources refer to the same route.

The APB destination name is mapped to a waypoint number; its heading-to-steer
field has no equivalent in these PGNs and is not forwarded. Mixed true/magnetic
bearings require current magnetic variation to retain the origin bearing.

For a host-side conversion check after installing the `m5stack-atom-dev`
dependencies, run `sh test/run_apb_to_n2k.sh` from the project root.

## NMEA 2000 to NMEA 0183 APB

PGN 129284 generates APB using the current shared cross-track error, which can
be updated by PGN 129283. APB is not emitted without a valid cross-track error,
position bearing and destination waypoint number. Navigation data must also
pass the existing shared-data validity and source-priority checks. Only
great-circle navigation with a valid true or magnetic bearing reference is
handled. The APB bearing reference matches the PGN; magnetic APB output does
not require magnetic variation.

The position-to-waypoint bearing is also used as heading-to-steer; PGN 129284
does not provide a separate heading-to-steer. The destination waypoint number
is used as the APB destination identifier. Unavailable arrival and perpendicular
statuses are emitted as empty fields.


# Special API requuests

## NMEA 2000 device list

Select the information icon next to NMEA 2000 State to view discovered
devices. Select a row to see its available details; Back returns to the list.

`/api/n2kDeviceList` returns a JSON array with the gateway's own device at
index 0. Other devices are at their NMEA 2000 source address plus one; unused
addresses are `null`. The gateway is not repeated in the rest of the array.
Fields use these abbreviated keys:

| Key | Meaning |
| --- | --- |
| `src` | Source address |
| `name` | 64-bit NMEA NAME as a 16-digit hexadecimal string |
| `uid`, `mfg` | Unique number and manufacturer code |
| `inst` | Device instance |
| `fn`, `cls`, `ind`, `si` | Device function, class, industry group and system instance |
| `ct`, `lt` | Elapsed milliseconds since device creation and last message (remote devices) |
| `n2k`, `pc` | NMEA 2000 version and product code |
| `mid`, `sw`, `mv`, `sn` | Model ID, software code, model version and serial code |
| `cert`, `load` | Certification level and load equivalency (in units of 50 mA) |
| `mi`, `id1`, `id2` | Manufacturer information and installation descriptions |
| `tx`, `rx` | Supported transmit and receive PGNs, when available |

Product and configuration information may not yet be available. The library's
unavailable product numeric values are reported as-is; empty or unavailable
strings are omitted. Missing PGN lists are omitted, while known empty lists are `[]`.
The gateway entry omits `ct`, `lt`, `tx` and `rx`, which are not provided by
the local device information API.
If the response cannot be serialized, the endpoint returns a JSON object with
an `error` field instead of a partial list.
