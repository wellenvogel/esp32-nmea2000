#include <cassert>
#include <math.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <NMEA0183Messages.h>
#include <N2kMessages.h>

const double degToRad = M_PI / 180.0;
const double nmTom = 1852.0;

extern "C" uint32_t millis() { return 100; }
extern "C" void delay(uint32_t) {}
uint32_t N2kMillis() { return 100; }

struct SNMEA0183Msg : tNMEA0183Msg {
    int sourceId;
    const char *line;
};

struct Item {
    double value = N2kDoubleNA;
    int source = -1;
    double getDataWithDefault(double fallback) const {
        return value == N2kDoubleNA ? fallback : value;
    }
    bool update(double next, int nextSource) {
        if (value != N2kDoubleNA && source >= 0 && source < nextSource) return false;
        value = next;
        source = nextSource;
        return true;
    }
};

struct BoatData {
    Item xte, btw, variation, dtw, latitude, longitude;
    Item *XTE = &xte, *BTW = &btw, *VAR = &variation, *DTW = &dtw;
    Item *WPLat = &latitude, *WPLon = &longitude;
};

#define LOG_DEBUG(...) do {} while (0)
class TestConverter {
public:
    BoatData data;
    BoatData *boatData = &data;
    std::vector<tN2kMsg> sent;
    std::map<std::string, uint8_t> waypoints;
    bool updateDouble(Item *item, double value, int source) {
        return value != NMEA0183DoubleNA && item->update(value, source);
    }
    tN2kXTEMode xteMode(char mode) {
        switch (mode) {
            case 'D': return N2kxtem_Differential;
            case 'E': return N2kxtem_Estimated;
            case 'M': return N2kxtem_Manual;
            case 'S': return N2kxtem_Simulator;
            default: return N2kxtem_Autonomous;
        }
    }
    uint8_t getWaypointId(const char *name) {
        if (!waypoints.count(name)) waypoints[name] = waypoints.size() + 2;
        return waypoints[name];
    }
    void send(tN2kMsg &msg, int) { sent.push_back(msg); }

#include "convert_apb.inc"
};

static void run(TestConverter &converter, const char *fields, int source = 1, const char *sentence = "APB") {
    tNMEA0183Msg builder;
    assert(builder.Init(sentence, "GP"));
    std::string input(fields);
    size_t start = 0;
    do {
        size_t end = input.find(',', start);
        assert(builder.AddStrField(input.substr(start, end - start).c_str()));
        if (end == std::string::npos) break;
        start = end + 1;
    } while (true);
    char wire[100];
    assert(builder.GetMessage(wire, sizeof(wire)));
    SNMEA0183Msg message;
    message.sourceId = source;
    message.line = wire;
    assert(message.SetMessage(wire));
    if (std::string(sentence) == "RMB") converter.convertRMB(message);
    else converter.convertAPB(message);
}

static bool near(double a, double b, double tolerance = 0.00011) {
    return fabs(a - b) < tolerance;
}

int main() {
    TestConverter normalizer;
    assert(NMEA0183IsNA(normalizer.normalizeBearing(NMEA0183DoubleNA)));
    assert(fabs(normalizer.normalizeBearing(-M_PI / 2) - 3 * M_PI / 2) < 1e-10);
    assert(fabs(normalizer.normalizeBearing(2 * M_PI)) < 1e-10);
    assert(fabs(normalizer.normalizeBearing(5 * M_PI / 2) - M_PI / 2) < 1e-10);

    tNMEA0183Msg parserInput;
    const char *fields[] = {"A", "A", "", "R", "N", "V", "V",
                            "", "T", "WP", "", "T", "", "T"};
    assert(parserInput.Init("APB", "GP"));
    for (const char *field : fields) assert(parserInput.AddStrField(field));
    tAPB parsed;
    parsed.mode = 'D';
    assert(NMEA0183ParseAPB_nc(parserInput, parsed));
    assert(parsed.mode == '\0');
    assert(NMEA0183IsNA(parsed.xte) && NMEA0183IsNA(parsed.botw));
    assert(NMEA0183IsNA(parsed.btw) && NMEA0183IsNA(parsed.headingToSteer));
    assert(parserInput.AddStrField("D"));
    assert(NMEA0183ParseAPB_nc(parserInput, parsed) && parsed.mode == 'D');

    TestConverter converter;
    converter.data.dtw.update(2000, 1);
    converter.data.latitude.update(54, 1);
    converter.data.longitude.update(13, 1);
    run(converter, "A,A,0.10,R,N,A,V,011,T,DEST,045,T,050,T,D");
    assert(converter.sent.size() == 2);
    unsigned char sid;
    tN2kXTEMode mode;
    bool terminated;
    double xte;
    assert(ParseN2kXTE(converter.sent[0], sid, mode, terminated, xte));
    assert(mode == N2kxtem_Differential && !terminated && near(xte, -185.2, 0.01));
    double distance, eta, origin, position, lat, lon, velocity;
    tN2kHeadingReference ref;
    tN2kDistanceCalculationType calculation;
    bool perpendicular, arrival;
    int16_t date;
    uint32_t originId, destinationId;
    assert(ParseN2kNavigationInfo(converter.sent[1], sid, distance, ref, perpendicular,
        arrival, calculation, eta, date, origin, position, originId, destinationId,
        lat, lon, velocity));
    assert(ref == N2khr_true && arrival && !perpendicular);
    assert(near(origin, 11 * degToRad) && near(position, 45 * degToRad));
    assert(distance == 2000 && lat == 54 && lon == 13 && originId == N2kUInt32NA);
    assert(destinationId == 2 && N2kIsNA(velocity));
    assert(near(converter.data.btw.value, 45 * degToRad));

    const char *rmbFields[] = {
        "A,0.10,R,ORIGIN,DEST,5400.000,N,01300.000,E,1.0,360,0.0,V",
        "A,0.10,R,ORIGIN,DEST,5400.000,N,01300.000,E,1.0,-10,0.0,V",
        "A,0.10,R,ORIGIN,DEST,5400.000,N,01300.000,E,1.0,370,0.0,V",
        "A,0.10,R,ORIGIN,DEST,5400.000,N,01300.000,E,1.0,45,0.0,V"
    };
    const double rmbBearings[] = {0, 350 * degToRad, 10 * degToRad, 45 * degToRad};
    for (size_t i = 0; i < sizeof(rmbBearings) / sizeof(rmbBearings[0]); ++i) {
        TestConverter rmb;
        run(rmb, rmbFields[i], 1, "RMB");
        assert(rmb.sent.size() == 3 && rmb.sent[1].PGN == 129284);
        assert(ParseN2kNavigationInfo(rmb.sent[1], sid, distance, ref, perpendicular,
            arrival, calculation, eta, date, origin, position, originId, destinationId,
            lat, lon, velocity));
        assert(near(rmb.data.btw.value, rmbBearings[i], 1e-9));
        assert(near(position, rmbBearings[i]));
    }

    TestConverter autonomous;
    run(autonomous, "A,A,0.10,L,N,A,V,011,T,DEST,045,T,050,T");
    assert(autonomous.sent.size() == 2);
    assert(ParseN2kXTE(autonomous.sent[0], sid, mode, terminated, xte));
    assert(mode == N2kxtem_Autonomous && near(xte, 185.2, 0.01));

    TestConverter mixed;
    mixed.data.variation.update(10 * degToRad, 1);
    run(mixed, "A,A,0.20,L,N,V,A,350,M,WP,020,T,020,T");
    assert(mixed.sent.size() == 2);
    assert(ParseN2kNavigationInfo(mixed.sent[1], sid, distance, ref, perpendicular,
        arrival, calculation, eta, date, origin, position, originId, destinationId,
        lat, lon, velocity));
    assert(near(origin, 0) && perpendicular && !arrival);
    assert(N2kIsNA(distance) && N2kIsNA(lat) && N2kIsNA(lon));

    TestConverter unavailable;
    run(unavailable, "A,A,0.20,L,N,,,010,M,WP,020,T,020,T");
    assert(unavailable.sent.size() == 2);
    assert(ParseN2kNavigationInfo(unavailable.sent[1], sid, distance, ref, perpendicular,
        arrival, calculation, eta, date, origin, position, originId, destinationId,
        lat, lon, velocity));
    assert(N2kIsNA(origin) && (unavailable.sent[1].Data[5] & 0x3c) == 0x3c);

    TestConverter magnetic;
    magnetic.data.variation.update(10 * degToRad, 1);
    run(magnetic, "A,A,0.20,L,N,V,V,005,T,WP,020,M,020,M");
    assert(magnetic.sent.size() == 2);
    assert(ParseN2kNavigationInfo(magnetic.sent[1], sid, distance, ref, perpendicular,
        arrival, calculation, eta, date, origin, position, originId, destinationId,
        lat, lon, velocity));
    assert(ref == N2khr_magnetic && near(origin, 355 * degToRad));
    assert(near(magnetic.data.btw.value, 30 * degToRad));

    TestConverter invalid;
    run(invalid, "V,A,0.10,R,N,V,V,011,T,DEST,045,T,050,T");
    run(invalid, "A,A,,R,N,V,V,011,T,DEST,045,T,050,T");
    run(invalid, "A,A,,L,N,V,V,011,T,DEST,045,T,050,T");
    run(invalid, "A,A,0.10,R,N,V,V,011,T,DEST,045,T,050,T,N");
    run(invalid, "A,A,20000,R,N,V,V,011,T,DEST,045,T,050,T");
    run(invalid, "A,A,0.10,R,K,V,V,011,T,DEST,045,T,050,T");
    assert(invalid.sent.empty());

    TestConverter missingBearing;
    run(missingBearing, "A,A,0.10,R,N,V,V,011,T,DEST,,T,050,T");
    assert(missingBearing.sent.size() == 1 && missingBearing.sent[0].PGN == 129283);

    TestConverter missingOrigin;
    run(missingOrigin, "A,A,0.10,L,N,A,A,,,WP,020,T,020,T");
    assert(missingOrigin.sent.size() == 2);
    assert(ParseN2kNavigationInfo(missingOrigin.sent[1], sid, distance, ref,
        perpendicular, arrival, calculation, eta, date, origin, position, originId,
        destinationId, lat, lon, velocity));
    assert(N2kIsNA(origin) && near(position, 20 * degToRad));

    TestConverter invalidBearing;
    run(invalidBearing, "A,A,0.10,L,N,A,A,010,T,WP,361,T,020,T");
    assert(invalidBearing.sent.size() == 1 && invalidBearing.sent[0].PGN == 129283);

    TestConverter magneticNoVariation;
    run(magneticNoVariation, "A,A,0.10,L,N,A,A,350,T,WP,020,M,020,M");
    assert(magneticNoVariation.sent.size() == 2);
    assert(ParseN2kNavigationInfo(magneticNoVariation.sent[1], sid, distance, ref,
        perpendicular, arrival, calculation, eta, date, origin, position, originId,
        destinationId, lat, lon, velocity));
    assert(ref == N2khr_magnetic && N2kIsNA(origin));
    assert(N2kIsNA(magneticNoVariation.data.btw.value));

    TestConverter invalidOrigin;
    run(invalidOrigin, "A,A,0.10,L,N,A,A,361,T,WP,020,T,020,T");
    assert(invalidOrigin.sent.size() == 1 && N2kIsNA(invalidOrigin.data.btw.value));

    TestConverter priority;
    priority.data.xte.update(42, 0);
    priority.data.btw.update(1, 0);
    run(priority, "A,A,0.10,R,N,V,V,011,T,DEST,045,T,050,T");
    assert(priority.sent.empty() && priority.data.xte.value == 42 && priority.data.btw.value == 1);
}
