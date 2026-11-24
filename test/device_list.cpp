#include <ArduinoJson.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#define F(value) value
#define N2kMaxBusDevices 254
#define Max_N2kModelID_len 32
#define Max_N2kSwCode_len 32
#define Max_N2kModelVersion_len 32
#define Max_N2kModelSerialCode_len 32
#define Max_N2kConfigurationInfoField_len 71
using String = std::string;
unsigned long millis() { return 10000; }

struct FakeIdentity {
  uint64_t GetName() const { return 0x123456789abcdef0ULL; }
  uint32_t GetUniqueNumber() const { return 42; }
  uint16_t GetManufacturerCode() const { return 2046; }
  uint8_t GetDeviceInstance() const { return 3; }
  uint8_t GetDeviceFunction() const { return 130; }
  uint8_t GetDeviceClass() const { return 25; }
  uint8_t GetIndustryGroup() const { return 4; }
  uint8_t GetSystemInstance() const { return 1; }
};

struct FakeNMEA2000 {
  uint8_t GetN2kSource() const { return 2; }
  FakeIdentity GetDeviceInformation() const { return {}; }
  unsigned short GetN2kVersion() const { return 2101; }
  unsigned short GetProductCode() const { return 100; }
  unsigned char GetCertificationLevel() const { return 1; }
  unsigned char GetLoadEquivalency() const { return 2; }
  void GetModelID(char *buf, size_t size) const { snprintf(buf,size,"gateway"); }
  void GetSwCode(char *buf, size_t size) const { snprintf(buf,size,"firmware"); }
  void GetModelVersion(char *buf, size_t size) const { snprintf(buf,size,"version"); }
  void GetModelSerialCode(char *buf, size_t size) const { snprintf(buf,size,"1"); }
  void GetManufacturerInformation(char *buf, size_t size) const { snprintf(buf,size,"vendor"); }
  void GetInstallationDescription1(char *buf, size_t size) const { snprintf(buf,size,"bridge"); }
  void GetInstallationDescription2(char *buf, size_t size) const { if (size) buf[0]='\0'; }
} NMEA2000;

struct FakeDevice {
  uint8_t source;
  uint64_t name;
  const unsigned long *tx;
  const unsigned long *rx;
  bool emptyStrings;
  uint8_t GetSource() const { return source; }
  uint64_t GetName() const { return name; }
  uint32_t GetUniqueNumber() const { return 123456; }
  uint16_t GetManufacturerCode() const { return 2046; }
  uint8_t GetDeviceInstance() const { return 25; }
  uint8_t GetDeviceInstanceLower() const { return 1; }
  uint8_t GetDeviceInstanceUpper() const { return 3; }
  uint8_t GetDeviceFunction() const { return 130; }
  uint8_t GetDeviceClass() const { return 25; }
  uint8_t GetIndustryGroup() const { return 4; }
  uint8_t GetSystemInstance() const { return 2; }
  unsigned long GetCreateTime() const { return 1234; }
  unsigned short GetN2kVersion() const { return 1300; }
  unsigned short GetProductCode() const { return 100; }
  const char *GetModelID() const { return emptyStrings ? "" : "model"; }
  const char *GetSwCode() const { return emptyStrings ? nullptr : "software"; }
  const char *GetModelVersion() const { return emptyStrings ? "" : "version"; }
  const char *GetModelSerialCode() const { return emptyStrings ? nullptr : "serial"; }
  unsigned short GetCertificationLevel() const { return 1; }
  unsigned short GetLoadEquivalency() const { return 2; }
  const char *GetManufacturerInformation() const { return emptyStrings ? "" : "manufacturer"; }
  const char *GetInstallationDescription1() const { return emptyStrings ? nullptr : "first"; }
  const char *GetInstallationDescription2() const { return emptyStrings ? "" : nullptr; }
  const unsigned long *GetTransmitPGNs() const { return tx; }
  const unsigned long *GetReceivePGNs() const { return rx; }
};

struct FakeDeviceList {
  FakeDevice *devices[N2kMaxBusDevices] = {};
  void ReadResetIsListUpdated() {}
  const FakeDevice *FindDeviceBySource(int source) const { return devices[source]; }
  unsigned long GetDeviceLastMessageTime(int) const { return 5678; }
} n2kDeviceList;

struct GwLog { static const int ERROR=0; };
struct FakeLogger {
  void logDebug(int, const char *) { assert(false); }
} logger;

class GwRequestMessage {
protected:
  std::string result;
  virtual void processRequest() = 0;
public:
  GwRequestMessage(const char *, const char *) {}
  std::string run() { processRequest(); return result; }
};

#include "device_list_request.inc"

int main() {
  N2kDeviceListRequest request;
  DynamicJsonDocument ownParsed(4096);
  assert(!deserializeJson(ownParsed,request.run()));
  assert(ownParsed.size() == 1 && ownParsed[0]["src"] == 2);
  assert(ownParsed[0]["name"] == "123456789abcdef0");
  assert(ownParsed[0]["uid"] == 42 && ownParsed[0]["mfg"] == 2046);
  assert(ownParsed[0]["inst"] == 3 && ownParsed[0]["fn"] == 130);
  assert(ownParsed[0]["cls"] == 25 && ownParsed[0]["ind"] == 4 && ownParsed[0]["si"] == 1);
  assert(ownParsed[0]["n2k"] == 2101 && ownParsed[0]["pc"] == 100);
  assert(ownParsed[0]["cert"] == 1 && ownParsed[0]["load"] == 2);
  assert(ownParsed[0]["mid"] == "gateway" && ownParsed[0]["sw"] == "firmware");
  assert(ownParsed[0]["mv"] == "version" && ownParsed[0]["sn"] == "1");
  assert(ownParsed[0]["mi"] == "vendor");
  assert(ownParsed[0]["id1"] == "bridge" && !ownParsed[0].containsKey("id2"));
  assert(!ownParsed[0].containsKey("ct") && !ownParsed[0].containsKey("tx"));

  const unsigned long tx[] = {129283, 129284, 0};
  const unsigned long rx[] = {127250, 0};
  FakeDevice first = {1, 0xfedcba9876543210ULL, tx, rx, false};
  FakeDevice duplicate = {2, 0xfedcba9876543210ULL, tx, rx, false};
  FakeDevice last = {253, 0x0123456789abcdefULL, nullptr, nullptr, true};
  n2kDeviceList.devices[1] = &first;
  n2kDeviceList.devices[2] = &duplicate;
  n2kDeviceList.devices[253] = &last;
  N2kDeviceListRequest populated;
  std::string json=populated.run();
  DynamicJsonDocument parsed(32768);
  assert(!deserializeJson(parsed, json));
  assert(parsed.is<JsonArray>() && parsed.size() == 255);
  assert(parsed[0]["src"] == 2 && parsed[1].isNull() && parsed[3].isNull());
  JsonObject firstInfo=parsed[2];
  assert(firstInfo.size() == 23);
  assert(firstInfo["src"] == 1 && firstInfo["cls"] == 25);
  assert(firstInfo["name"] == "fedcba9876543210");
  assert(firstInfo["uid"] == 123456 && firstInfo["mfg"] == 2046);
  assert(firstInfo["inst"] == 25);
  assert(!firstInfo.containsKey("instLower") && !firstInfo.containsKey("instUpper"));
  assert(firstInfo["fn"] == 130 && firstInfo["ind"] == 4 && firstInfo["si"] == 2);
  assert(firstInfo["ct"] == 8766 && firstInfo["lt"] == 4322);
  assert(firstInfo["n2k"] == 1300 && firstInfo["pc"] == 100);
  assert(firstInfo["mid"] == "model" && firstInfo["sw"] == "software");
  assert(firstInfo["mv"] == "version" && firstInfo["sn"] == "serial");
  assert(firstInfo["cert"] == 1 && firstInfo["load"] == 2);
  assert(firstInfo["mi"] == "manufacturer" && firstInfo["id1"] == "first");
  assert(!firstInfo.containsKey("id2"));
  assert(firstInfo["tx"].size() == 2 && firstInfo["tx"][1] == 129284);
  assert(firstInfo["rx"].size() == 1 && firstInfo["rx"][0] == 127250);
  assert(parsed[254]["name"] == "0123456789abcdef");
  JsonObject lastInfo=parsed[254];
  assert(lastInfo.size() == 15);
  assert(!lastInfo.containsKey("mid") && !lastInfo.containsKey("sw"));
  assert(!lastInfo.containsKey("mv") && !lastInfo.containsKey("sn"));
  assert(!lastInfo.containsKey("mi") && !lastInfo.containsKey("id1") && !lastInfo.containsKey("id2"));
  assert(!lastInfo.containsKey("tx") && !lastInfo.containsKey("rx"));
}
