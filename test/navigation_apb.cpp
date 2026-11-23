#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

#include "NavigationAPB.h"

extern "C" uint32_t millis() { return 1; }
extern "C" void delay(uint32_t) {}

static void checkFields(const tNMEA0183Msg &message, const char *const *expected) {
    assert(message.FieldCount() == 14);
    for (unsigned int i = 0; i < 14; ++i) {
        assert(strcmp(message.Field(i), expected[i]) == 0);
    }

    char wire[100];
    assert(message.GetMessage(wire, sizeof(wire)));
    tNMEA0183Msg parsed;
    assert(parsed.SetMessage(wire));
    assert(parsed.IsMessageCode("APB"));
    assert(parsed.FieldCount() == 14);
    for (unsigned int i = 0; i < 14; ++i) {
        assert(strcmp(parsed.Field(i), expected[i]) == 0);
    }
}

int main() {
    assert(navigationApbReferenceUsable(0));
    assert(navigationApbReferenceUsable(1));
    assert(!navigationApbReferenceUsable(2));
    assert(!navigationApbReferenceUsable(3));
    assert(!navigationApbReferenceUsable(0x40));
    assert(!navigationApbReferenceUsable(0x80));

    tNMEA0183Msg message;
    const char *trueFields[] = {
        "A", "A", "0.1250", "L", "N", "A", "V", "12.3", "T", "42",
        "345.6", "T", "345.6", "T"
    };
    assert(buildNavigationApb(message, "GP", 0.125, 12.3, 345.6, "T", 42, 1, 0));
    checkFields(message, trueFields);

    const char *magneticFields[] = {
        "A", "A", "12.3456", "R", "N", "", "", "", "M", "4294967294",
        "0.0", "M", "0.0", "M"
    };
    assert(buildNavigationApb(message, "II", -12.3456, NMEA0183DoubleNA, 0, "M",
                              0xfffffffe, 2, 3));
    checkFields(message, magneticFields);

    assert(!buildNavigationApb(message, "GP", NMEA0183DoubleNA, 1, 2, "T", 1, 0, 0));
    assert(!buildNavigationApb(message, "GP", 0, 1, NMEA0183DoubleNA, "T", 1, 0, 0));
    assert(!buildNavigationApb(message, "GP", 0, 1, 2, "T", NMEA0183UInt32NA, 0, 0));
    assert(!buildNavigationApb(message, "GP", std::numeric_limits<double>::quiet_NaN(),
                               1, 2, "T", 1, 0, 0));
    assert(!buildNavigationApb(message, "GP", 0, 1,
                               std::numeric_limits<double>::infinity(), "T", 1, 0, 0));
}
