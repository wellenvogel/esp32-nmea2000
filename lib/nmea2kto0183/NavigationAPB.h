#ifndef NAVIGATION_APB_H
#define NAVIGATION_APB_H

#include <math.h>
#include <NMEA0183Msg.h>

inline bool navigationApbReferenceUsable(unsigned char flags) {
    return (flags & 0x03) <= 1 && (flags & 0xc0) == 0;
}

inline const char *navigationApbStatus(unsigned int status) {
    if (status == 0) return "V";
    if (status == 1) return "A";
    return "";
}

inline bool buildNavigationApb(tNMEA0183Msg &message, const char *talkerId,
                               double xteNm, double originBearing,
                               double positionBearing, const char *bearingReference,
                               uint32_t destination, unsigned int arrival,
                               unsigned int perpendicular) {
    if (NMEA0183IsNA(xteNm) || !isfinite(xteNm) ||
        NMEA0183IsNA(positionBearing) || !isfinite(positionBearing) ||
        NMEA0183IsNA(destination)) return false;

    const char *direction = xteNm < 0 ? "R" : "L";
    if (!message.Init("APB", talkerId)) return false;
    if (!message.AddStrField("A")) return false;
    if (!message.AddStrField("A")) return false;
    if (!message.AddDoubleField(fabs(xteNm), 1.0, "%.4f")) return false;
    if (!message.AddStrField(direction)) return false;
    if (!message.AddStrField("N")) return false;
    if (!message.AddStrField(navigationApbStatus(arrival))) return false;
    if (!message.AddStrField(navigationApbStatus(perpendicular))) return false;
    if (!message.AddDoubleField(originBearing)) return false;
    if (!message.AddStrField(bearingReference)) return false;
    if (!message.AddUInt32Field(destination)) return false;
    if (!message.AddDoubleField(positionBearing)) return false;
    if (!message.AddStrField(bearingReference)) return false;
    if (!message.AddDoubleField(positionBearing)) return false;
    if (!message.AddStrField(bearingReference)) return false;
    return true;
}

#endif
