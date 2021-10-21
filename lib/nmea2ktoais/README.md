# NMEA2000 -> NMEA0183 AIS converter  v1.0.0

Import from https://github.com/ronzeiller/NMEA0183-AIS

NMEA0183  AIS library © Ronnie Zeiller, www.zeiller.eu

Addendum for NMEA2000 and NMEA0183 Library from Timo Lappalainen https://github.com/ttlappalainen


## Conversions:

- NMEA2000 PGN 129038 => AIS CLASS A Position Report (Message Type 1) 1.) 2.) 3.)
- NMEA2000 PGN 129039 => AIS Class B Position Report, Message Type 18
- NMEA2000 PGN 129794 => AIS Class A Ship Static and Voyage related data, Message Type 5 4.)
- NMEA2000 PGN 129809 => AIS Class B "CS" Static Data Report, making a list of UserID (MMSI) and Ship Names used for Message 24 Part A
- NMEA2000 PGN 129810 => AIS Class B "CS" Static Data Report, Message 24 Part A+B

### Remarks
1. Message Type could be set to 1 or 3 (identical messages) on demand
2. Maneuver Indicator (not part of NMEA2000 PGN 129038) => will be set to 0 (default)
3. Radio Status (not part of NMEA2000 PGN 129038) => will be set to 0
4. AIS Version (not part of NMEA2000 PGN 129794) => will be set to 1

## Dependencies

To use this library you need also:

   - NMEA2000 library

   - NMEA0183 library

   - Related CAN libraries.

## License

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
