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
