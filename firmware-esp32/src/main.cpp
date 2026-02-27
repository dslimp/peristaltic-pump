#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <array>
#include <cmath>
#include <cstring>
#include <HTTPClient.h>
#include <time.h>
#include <Wire.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include "PumpController.h"

namespace cfg {
constexpr char kFirmwareVersion[] = "0.2.11-esp32";

constexpr char kApSsid[] = "PeristalticPump-Setup";
constexpr char kApPass[] = "pump12345";
constexpr uint16_t kApPortalTimeoutSec = 180;
constexpr char kDebugWifiSsid[] = "nh";          // temporary for debug
constexpr char kDebugWifiPass[] = "Fx110011";    // temporary for debug
constexpr uint16_t kDebugWifiConnectTimeoutMs = 12000;

#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr uint8_t kPinStep = 4;
constexpr uint8_t kPinDir = 5;
constexpr uint8_t kPinEnable = 6;
#else
constexpr uint8_t kPinStep = 25;
constexpr uint8_t kPinDir = 26;
constexpr uint8_t kPinEnable = 27;
#endif

constexpr uint8_t kPinI2cSda = 8;   // YD-ESP32-23 / ESP32-S3 typical SDA
constexpr uint8_t kPinI2cScl = 9;   // YD-ESP32-23 / ESP32-S3 typical SCL
constexpr uint8_t kOledAddr = 0x3C;
constexpr uint8_t kOledWidth = 128;
constexpr uint8_t kOledHeight = 64;
constexpr uint16_t kOledRefreshMs = 500;

constexpr int kLedcChannel = 0;
constexpr int kLedcResolutionBits = 8;
constexpr int kMicroStepping = 8;
constexpr float kStepAngleDeg = 1.8f;
constexpr uint16_t kControlTickMs = 10;
constexpr uint16_t kSavePeriodMs = 5000;
constexpr uint8_t kMaxSchedules = 8;
constexpr uint8_t kMaxScheduleNameLen = 32;
constexpr uint8_t kBaseMotors = 1;
constexpr uint8_t kExpansionMaxMotors = 4;
constexpr uint8_t kMaxMotors = kBaseMotors + kExpansionMaxMotors;
constexpr uint8_t kExpansionI2cAddrFrom = 0x20;
constexpr uint8_t kExpansionI2cAddrTo = 0x2F;
constexpr uint16_t kExpansionDiscoveryMs = 2000;
constexpr uint16_t kExpansionPollMs = 300;
constexpr uint8_t kMaxFirmwareReleases = 1;
constexpr uint16_t kFirmwareReleasesDocBytes = 65520;
constexpr uint16_t kFirmwareReleaseDocBytes = 32768;
constexpr char kDefaultFirmwareRepo[] = "dslimp/peristaltic-pump";
constexpr char kDefaultFirmwareAsset[] = "firmware.bin";
constexpr char kDefaultFirmwareFsAsset[] = "littlefs.bin";
}  // namespace cfg

namespace exproto {
constexpr uint8_t kMagicA = 0x50;  // 'P'
constexpr uint8_t kMagicB = 0x58;  // 'X'
constexpr uint8_t kProtoVer = 1;
constexpr uint8_t kCmdHello = 0x01;
constexpr uint8_t kCmdGetState = 0x10;
constexpr uint8_t kCmdSetFlow = 0x20;
constexpr uint8_t kCmdStartDosing = 0x21;
constexpr uint8_t kCmdStop = 0x22;
constexpr uint8_t kCmdStart = 0x23;
constexpr uint8_t kCmdSetSettings = 0x24;
constexpr uint8_t kHelloRespLen = 6;
constexpr uint8_t kStateRespLen = 29;
}  // namespace exproto

WebServer server(80);
Preferences prefs;
WiFiManager wifiManager;
std::array<pump::PumpController, cfg::kMaxMotors> controllers = {
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
};
Adafruit_SSD1306 oled(cfg::kOledWidth, cfg::kOledHeight, &Wire, -1);

const float kStepsPerRevolution = (360.0f / cfg::kStepAngleDeg) * cfg::kMicroStepping;
uint32_t lastControlMs = 0;
uint32_t lastSaveMs = 0;
uint32_t lastOledMs = 0;
bool oledReady = false;
std::array<bool, cfg::kMaxMotors> preferredReverse = {false, false, false, false, false};
std::array<String, cfg::kMaxMotors> motorAliases = {"Motor 0", "Motor 1", "Motor 2", "Motor 3", "Motor 4"};
uint8_t selectedMotorId = 0;
bool webAuthEnabled = false;
String webAuthUser = "admin";
String webAuthPass = "admin";
String ntpServer = "time.google.com";
String uiLanguage = "en";
bool growthProgramEnabled = false;
bool phRegulationEnabled = false;
String firmwareRepo = cfg::kDefaultFirmwareRepo;
String firmwareAssetName = cfg::kDefaultFirmwareAsset;
String firmwareFsAssetName = cfg::kDefaultFirmwareFsAsset;
String expansionInterface = "i2c";
bool expansionEnabled = false;
uint8_t expansionMotorCount = 0;
bool expansionConnected = false;
uint8_t expansionI2cAddress = 0;
uint32_t lastExpansionDiscoveryMs = 0;
uint32_t lastExpansionPollMs = 0;

struct DoseScheduleEntry {
  bool enabled = false;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint16_t volumeMl = 0;
  bool reverse = false;
  uint8_t motorId = 0;
  char name[cfg::kMaxScheduleNameLen + 1] = {0};
  uint8_t weekdaysMask = 0x7F;  // bit0..6 => Mon..Sun
  int lastRunYDay = -1;
};

DoseScheduleEntry doseSchedules[cfg::kMaxSchedules];
int tzOffsetMinutes = 0;
bool getLocalTimeWithOffset(struct tm* outTm);
pump::PumpController& controllerById(uint8_t motorId);

bool githubHttpGet(const String& url, int* statusCode, String* body) {
  if (statusCode) *statusCode = 0;
  if (body) body->clear();
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "peristaltic-esp32");
  const int code = http.GET();
  if (statusCode) *statusCode = code;
  if (code > 0 && body) *body = http.getString();
  http.end();
  return code > 0;
}

bool performHttpOta(const String& url, int command, int* updateError, String* updateErrorString) {
  if (updateError) *updateError = 0;
  if (updateErrorString) updateErrorString->clear();
  const bool isHttps = url.startsWith("https://");
  const bool isHttp = url.startsWith("http://");
  if (!isHttps && !isHttp) {
    if (updateError) *updateError = -2;
    if (updateErrorString) *updateErrorString = "unsupported url scheme";
    return false;
  }

  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  if (isHttps) secureClient.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(60000);
  const bool beginOk = isHttps ? http.begin(secureClient, url) : http.begin(plainClient, url);
  if (!beginOk) {
    if (updateError) *updateError = -1;
    if (updateErrorString) *updateErrorString = "http begin failed";
    return false;
  }
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    if (updateError) *updateError = -104;
    if (updateErrorString) *updateErrorString = String("Wrong HTTP Code: ") + String(code);
    http.end();
    return false;
  }
  const int contentLength = http.getSize();
  if (contentLength <= 0) {
    if (updateError) *updateError = -101;
    if (updateErrorString) *updateErrorString = "content length missing";
    http.end();
    return false;
  }
  if (!Update.begin(static_cast<size_t>(contentLength), command)) {
    if (updateError) *updateError = Update.getError();
    if (updateErrorString) *updateErrorString = String(Update.errorString());
    http.end();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  const size_t written = Update.writeStream(*stream);
  if (written != static_cast<size_t>(contentLength)) {
    if (updateError) *updateError = Update.getError();
    if (updateErrorString) *updateErrorString = String(Update.errorString());
    Update.abort();
    http.end();
    return false;
  }
  if (!Update.end()) {
    if (updateError) *updateError = Update.getError();
    if (updateErrorString) *updateErrorString = String(Update.errorString());
    http.end();
    return false;
  }
  if (!Update.isFinished()) {
    if (updateError) *updateError = -105;
    if (updateErrorString) *updateErrorString = "update not finished";
    http.end();
    return false;
  }
  http.end();
  return true;
}

bool probeHttpUrl(const String& url, int* statusCode, int* contentLength, String* errorMessage) {
  if (statusCode) *statusCode = 0;
  if (contentLength) *contentLength = -1;
  if (errorMessage) errorMessage->clear();
  const bool isHttps = url.startsWith("https://");
  const bool isHttp = url.startsWith("http://");
  if (!isHttps && !isHttp) {
    if (errorMessage) *errorMessage = "unsupported url scheme";
    return false;
  }

  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  if (isHttps) secureClient.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(10000);
  const bool beginOk = isHttps ? http.begin(secureClient, url) : http.begin(plainClient, url);
  if (!beginOk) {
    if (errorMessage) *errorMessage = "http begin failed";
    return false;
  }
  const int code = http.GET();
  if (statusCode) *statusCode = code;
  if (code <= 0) {
    if (errorMessage) *errorMessage = String("Wrong HTTP Code: ") + String(code);
    http.end();
    return false;
  }
  if (contentLength) *contentLength = http.getSize();
  const bool ok = code == HTTP_CODE_OK;
  if (!ok && errorMessage) *errorMessage = String("Unexpected HTTP Code: ") + String(code);
  http.end();
  return ok;
}

bool isValidRepoSlug(const String& value) {
  if (value.length() < 3 || value.length() > 120) return false;
  const int slash = value.indexOf('/');
  if (slash <= 0 || slash >= value.length() - 1) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    const bool okChar = (c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') ||
                        c == '-' || c == '_' || c == '.' || c == '/';
    if (!okChar) return false;
  }
  return true;
}

bool isValidHttpUrl(const String& value) {
  if (value.length() < 12 || value.length() > 400) return false;
  if (!value.startsWith("http://") && !value.startsWith("https://")) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') return false;
  }
  return true;
}

bool pickFirmwareAssetUrl(const JsonObjectConst& release, const String& requestedAsset, String* assetName, String* url) {
  if (!release["assets"].is<JsonArrayConst>()) return false;
  JsonArrayConst assets = release["assets"].as<JsonArrayConst>();
  String firstBinName = "";
  String firstBinUrl = "";
  for (JsonObjectConst asset : assets) {
    const String name = asset["name"] | "";
    const String downloadUrl = asset["browser_download_url"] | "";
    if (name.length() == 0 || downloadUrl.length() == 0) continue;
    if (requestedAsset.length() > 0 && name == requestedAsset) {
      if (assetName) *assetName = name;
      if (url) *url = downloadUrl;
      return true;
    }
    if (firstBinUrl.length() == 0 && name.endsWith(".bin")) {
      firstBinName = name;
      firstBinUrl = downloadUrl;
    }
  }
  if (firstBinUrl.length() == 0) return false;
  if (assetName) *assetName = firstBinName;
  if (url) *url = firstBinUrl;
  return true;
}

bool pickFilesystemAssetUrl(const JsonObjectConst& release, const String& requestedAsset, String* assetName, String* url) {
  if (!release["assets"].is<JsonArrayConst>()) return false;
  JsonArrayConst assets = release["assets"].as<JsonArrayConst>();
  String fallbackName = "";
  String fallbackUrl = "";
  for (JsonObjectConst asset : assets) {
    const String name = asset["name"] | "";
    const String downloadUrl = asset["browser_download_url"] | "";
    if (name.length() == 0 || downloadUrl.length() == 0) continue;
    if (requestedAsset.length() > 0 && name == requestedAsset) {
      if (assetName) *assetName = name;
      if (url) *url = downloadUrl;
      return true;
    }
    if (fallbackUrl.length() == 0 && name.endsWith(".bin") && name.indexOf("littlefs") >= 0) {
      fallbackName = name;
      fallbackUrl = downloadUrl;
    }
  }
  if (fallbackUrl.length() == 0) return false;
  if (assetName) *assetName = fallbackName;
  if (url) *url = fallbackUrl;
  return true;
}

bool resolveFirmwareAsset(const String& repo, const String& mode, const String& tag, const String& requestedAsset,
                          String* resolvedTag, String* resolvedAssetName, String* assetUrl, String* error,
                          int* httpCode) {
  if (httpCode) *httpCode = 0;
  if (!isValidRepoSlug(repo)) {
    if (error) *error = "firmware repo must be in owner/repo format";
    return false;
  }
  String endpoint = String("https://api.github.com/repos/") + repo;
  if (mode == "latest") {
    endpoint += "/releases/latest";
  } else if (mode == "tag") {
    if (tag.length() == 0) {
      if (error) *error = "tag is required when mode=tag";
      return false;
    }
    endpoint += "/releases/tags/" + tag;
  } else {
    if (error) *error = "mode must be latest or tag";
    return false;
  }

  int code = 0;
  String payload;
  if (!githubHttpGet(endpoint, &code, &payload)) {
    if (error) *error = "github request failed";
    return false;
  }
  if (httpCode) *httpCode = code;
  if (code != 200) {
    if (error) *error = "github release request failed";
    return false;
  }
  DynamicJsonDocument releaseDoc(cfg::kFirmwareReleaseDocBytes);
  if (deserializeJson(releaseDoc, payload) != DeserializationError::Ok || !releaseDoc.is<JsonObject>()) {
    if (error) *error = "invalid github release response";
    return false;
  }

  JsonObjectConst release = releaseDoc.as<JsonObjectConst>();
  String foundUrl;
  String foundAsset;
  if (!pickFirmwareAssetUrl(release, requestedAsset, &foundAsset, &foundUrl)) {
    if (error) *error = "release has no matching .bin asset";
    return false;
  }
  if (resolvedTag) *resolvedTag = release["tag_name"] | "";
  if (resolvedAssetName) *resolvedAssetName = foundAsset;
  if (assetUrl) *assetUrl = foundUrl;
  return true;
}

bool resolveReleaseForUpdate(const String& repo, const String& mode, const String& tag,
                             const String& firmwareAsset, const String& filesystemAsset,
                             String* resolvedTag, String* resolvedFirmwareAsset, String* firmwareUrl,
                             String* resolvedFsAsset, String* fsUrl, String* error, int* httpCode) {
  if (httpCode) *httpCode = 0;
  if (!isValidRepoSlug(repo)) {
    if (error) *error = "firmware repo must be in owner/repo format";
    return false;
  }
  String endpoint = String("https://api.github.com/repos/") + repo;
  if (mode == "latest") {
    endpoint += "/releases/latest";
  } else if (mode == "tag") {
    if (tag.length() == 0) {
      if (error) *error = "tag is required when mode=tag";
      return false;
    }
    endpoint += "/releases/tags/" + tag;
  } else {
    if (error) *error = "mode must be latest or tag";
    return false;
  }

  int code = 0;
  String payload;
  if (!githubHttpGet(endpoint, &code, &payload)) {
    if (error) *error = "github request failed";
    return false;
  }
  if (httpCode) *httpCode = code;
  if (code != 200) {
    if (error) *error = "github release request failed";
    return false;
  }
  DynamicJsonDocument releaseDoc(cfg::kFirmwareReleaseDocBytes);
  if (deserializeJson(releaseDoc, payload) != DeserializationError::Ok || !releaseDoc.is<JsonObject>()) {
    if (error) *error = "invalid github release response";
    return false;
  }
  JsonObjectConst release = releaseDoc.as<JsonObjectConst>();
  if (resolvedTag) *resolvedTag = release["tag_name"] | "";
  if (!pickFirmwareAssetUrl(release, firmwareAsset, resolvedFirmwareAsset, firmwareUrl)) {
    if (error) *error = "release has no matching firmware .bin asset";
    return false;
  }
  if (!pickFilesystemAssetUrl(release, filesystemAsset, resolvedFsAsset, fsUrl)) {
    if (error) *error = "release has no matching littlefs .bin asset";
    return false;
  }
  return true;
}

String defaultMotorAlias(uint8_t motorId) {
  char alias[20];
  snprintf(alias, sizeof(alias), "Motor %u", motorId);
  return String(alias);
}

String normalizedMotorAlias(const String& value, uint8_t motorId) {
  String alias = value;
  alias.trim();
  if (alias.length() == 0) return defaultMotorAlias(motorId);
  const char* raw = alias.c_str();
  size_t safeLen = 0;
  while (raw[safeLen] != '\0' && safeLen < 24) {
    const uint8_t lead = static_cast<uint8_t>(raw[safeLen]);
    size_t seqLen = 1;
    if ((lead & 0x80) == 0x00) {
      seqLen = 1;
    } else if ((lead & 0xE0) == 0xC0) {
      seqLen = 2;
    } else if ((lead & 0xF0) == 0xE0) {
      seqLen = 3;
    } else if ((lead & 0xF8) == 0xF0) {
      seqLen = 4;
    } else {
      break;
    }
    if (safeLen + seqLen > 24) break;
    bool valid = true;
    for (size_t i = 1; i < seqLen; ++i) {
      const uint8_t cont = static_cast<uint8_t>(raw[safeLen + i]);
      if ((cont & 0xC0) != 0x80) {
        valid = false;
        break;
      }
    }
    if (!valid) break;
    safeLen += seqLen;
  }
  if (safeLen < alias.length()) alias.remove(safeLen);
  return alias;
}

void setScheduleName(DoseScheduleEntry& entry, const String& value) {
  String name = value;
  name.trim();
  const char* raw = name.c_str();
  size_t safeLen = 0;
  while (raw[safeLen] != '\0' && safeLen < cfg::kMaxScheduleNameLen) {
    const uint8_t lead = static_cast<uint8_t>(raw[safeLen]);
    size_t seqLen = 1;
    if ((lead & 0x80) == 0x00) {
      seqLen = 1;
    } else if ((lead & 0xE0) == 0xC0) {
      seqLen = 2;
    } else if ((lead & 0xF0) == 0xE0) {
      seqLen = 3;
    } else if ((lead & 0xF8) == 0xF0) {
      seqLen = 4;
    } else {
      break;
    }
    if (safeLen + seqLen > cfg::kMaxScheduleNameLen) break;
    bool valid = true;
    for (size_t i = 1; i < seqLen; ++i) {
      const uint8_t cont = static_cast<uint8_t>(raw[safeLen + i]);
      if ((cont & 0xC0) != 0x80) {
        valid = false;
        break;
      }
    }
    if (!valid) break;
    safeLen += seqLen;
  }
  if (safeLen < name.length()) name.remove(safeLen);
  if (name.length() == 0) name = "Rule";
  strncpy(entry.name, name.c_str(), cfg::kMaxScheduleNameLen);
  entry.name[cfg::kMaxScheduleNameLen] = '\0';
}

uint8_t frameCrc(const uint8_t* data, size_t lenWithoutCrc) {
  uint8_t crc = 0;
  for (size_t i = 0; i < lenWithoutCrc; ++i) crc ^= data[i];
  return crc;
}

bool i2cExchange(uint8_t addr, const uint8_t* tx, size_t txLen, uint8_t* rx, size_t rxLen) {
  Wire.beginTransmission(addr);
  for (size_t i = 0; i < txLen; ++i) {
    Wire.write(tx[i]);
  }
  const uint8_t code = Wire.endTransmission(rxLen > 0 ? false : true);
  if (code != 0) return false;
  if (rxLen == 0) return true;
  const int got = Wire.requestFrom(static_cast<int>(addr), static_cast<int>(rxLen), static_cast<int>(true));
  if (got != static_cast<int>(rxLen)) return false;
  for (size_t i = 0; i < rxLen; ++i) {
    rx[i] = Wire.read();
  }
  return true;
}

bool expansionReadState(uint8_t remoteMotorIdx) {
  if (!expansionConnected || remoteMotorIdx >= expansionMotorCount) return false;
  uint8_t tx[3] = {exproto::kCmdGetState, remoteMotorIdx, 0};
  tx[2] = frameCrc(tx, 2);
  uint8_t rx[exproto::kStateRespLen] = {0};
  if (!i2cExchange(expansionI2cAddress, tx, sizeof(tx), rx, sizeof(rx))) return false;
  if (rx[28] != frameCrc(rx, 28)) return false;

  auto& ctrl = controllerById(static_cast<uint8_t>(remoteMotorIdx + 1));
  auto& st = ctrl.mutableState();
  auto rd16 = [&](int pos) -> int16_t {
    return static_cast<int16_t>(static_cast<uint16_t>(rx[pos]) | (static_cast<uint16_t>(rx[pos + 1]) << 8));
  };
  auto rdU16 = [&](int pos) -> uint16_t {
    return static_cast<uint16_t>(rx[pos]) | (static_cast<uint16_t>(rx[pos + 1]) << 8);
  };
  auto rdU32 = [&](int pos) -> uint32_t {
    return static_cast<uint32_t>(rx[pos]) |
           (static_cast<uint32_t>(rx[pos + 1]) << 8) |
           (static_cast<uint32_t>(rx[pos + 2]) << 16) |
           (static_cast<uint32_t>(rx[pos + 3]) << 24);
  };

  st.mode = (rx[0] == 1) ? pump::Mode::DOSING : pump::Mode::FLOW;
  st.running = rx[1] != 0;
  st.targetSpeed = static_cast<float>(rd16(2)) / 10.0f;
  st.currentSpeed = static_cast<float>(rd16(4)) / 10.0f;
  st.dosingRemainingMl = static_cast<float>(rdU16(6));
  st.totalMotorUptimeSec = rdU32(8);
  st.totalPumpedVolumeL = static_cast<double>(rdU32(12)) / 1000.0;
  st.totalHoseVolumeL = static_cast<double>(rdU32(16)) / 1000.0;
  st.mlPerRevCw = static_cast<float>(rdU16(20)) / 100.0f;
  st.mlPerRevCcw = static_cast<float>(rdU16(22)) / 100.0f;
  st.dosingSpeed = static_cast<float>(rdU16(24)) / 10.0f;
  ctrl.setMaxSpeed(static_cast<float>(rdU16(26)) / 10.0f);
  return true;
}

bool expansionCommandNoResp(uint8_t cmd, const uint8_t* payload, size_t payloadLen) {
  if (!expansionConnected) return false;
  uint8_t frame[20] = {0};
  if (payloadLen + 2 > sizeof(frame)) return false;
  frame[0] = cmd;
  for (size_t i = 0; i < payloadLen; ++i) frame[1 + i] = payload[i];
  frame[1 + payloadLen] = frameCrc(frame, 1 + payloadLen);
  return i2cExchange(expansionI2cAddress, frame, payloadLen + 2, nullptr, 0);
}

bool expansionSetFlow(uint8_t remoteMotorIdx, float lph, bool reverse) {
  uint8_t p[4] = {0};
  const int16_t lphX10 = static_cast<int16_t>(roundf(fabsf(lph) * 10.0f));
  p[0] = remoteMotorIdx;
  p[1] = static_cast<uint8_t>(lphX10 & 0xFF);
  p[2] = static_cast<uint8_t>((lphX10 >> 8) & 0xFF);
  p[3] = reverse ? 1 : 0;
  return expansionCommandNoResp(exproto::kCmdSetFlow, p, sizeof(p));
}

bool expansionStartDosing(uint8_t remoteMotorIdx, uint16_t volumeMl, bool reverse) {
  uint8_t p[4] = {0};
  p[0] = remoteMotorIdx;
  p[1] = static_cast<uint8_t>(volumeMl & 0xFF);
  p[2] = static_cast<uint8_t>((volumeMl >> 8) & 0xFF);
  p[3] = reverse ? 1 : 0;
  return expansionCommandNoResp(exproto::kCmdStartDosing, p, sizeof(p));
}

bool expansionStart(uint8_t remoteMotorIdx) {
  uint8_t p[1] = {remoteMotorIdx};
  return expansionCommandNoResp(exproto::kCmdStart, p, sizeof(p));
}

bool expansionStop(uint8_t remoteMotorIdx) {
  uint8_t p[1] = {remoteMotorIdx};
  return expansionCommandNoResp(exproto::kCmdStop, p, sizeof(p));
}

bool expansionSetSettings(uint8_t remoteMotorIdx, float mlCw, float mlCcw, float dosingFlowLph, float maxFlowLph) {
  uint8_t p[9] = {0};
  const uint16_t mlCwX100 = static_cast<uint16_t>(roundf(fmaxf(mlCw, 0.0f) * 100.0f));
  const uint16_t mlCcwX100 = static_cast<uint16_t>(roundf(fmaxf(mlCcw, 0.0f) * 100.0f));
  const uint16_t dosingLphX10 = static_cast<uint16_t>(roundf(fmaxf(dosingFlowLph, 0.0f) * 10.0f));
  const uint16_t maxLphX10 = static_cast<uint16_t>(roundf(fmaxf(maxFlowLph, 0.0f) * 10.0f));
  p[0] = remoteMotorIdx;
  p[1] = static_cast<uint8_t>(mlCwX100 & 0xFF);
  p[2] = static_cast<uint8_t>((mlCwX100 >> 8) & 0xFF);
  p[3] = static_cast<uint8_t>(mlCcwX100 & 0xFF);
  p[4] = static_cast<uint8_t>((mlCcwX100 >> 8) & 0xFF);
  p[5] = static_cast<uint8_t>(dosingLphX10 & 0xFF);
  p[6] = static_cast<uint8_t>((dosingLphX10 >> 8) & 0xFF);
  p[7] = static_cast<uint8_t>(maxLphX10 & 0xFF);
  p[8] = static_cast<uint8_t>((maxLphX10 >> 8) & 0xFF);
  return expansionCommandNoResp(exproto::kCmdSetSettings, p, sizeof(p));
}

bool discoverExpansionI2c() {
  if (!expansionEnabled || expansionInterface != "i2c") return false;
  uint8_t tx[2] = {exproto::kCmdHello, 0};
  tx[1] = frameCrc(tx, 1);
  uint8_t rx[exproto::kHelloRespLen] = {0};

  for (uint8_t addr = cfg::kExpansionI2cAddrFrom; addr <= cfg::kExpansionI2cAddrTo; ++addr) {
    if (!i2cExchange(addr, tx, sizeof(tx), rx, sizeof(rx))) continue;
    if (rx[5] != frameCrc(rx, 5)) continue;
    if (rx[0] != exproto::kMagicA || rx[1] != exproto::kMagicB || rx[2] != exproto::kProtoVer) continue;
    const uint8_t discovered = rx[3] > cfg::kExpansionMaxMotors ? cfg::kExpansionMaxMotors : rx[3];
    if (discovered == 0) continue;
    expansionI2cAddress = addr;
    expansionConnected = true;
    expansionMotorCount = discovered;
    return true;
  }
  expansionConnected = false;
  return false;
}

void refreshExpansionState() {
  if (!expansionEnabled || expansionInterface != "i2c") {
    expansionConnected = false;
    if (selectedMotorId > 0) selectedMotorId = 0;
    return;
  }
  const uint32_t now = millis();
  if (!expansionConnected && (now - lastExpansionDiscoveryMs >= cfg::kExpansionDiscoveryMs)) {
    lastExpansionDiscoveryMs = now;
    discoverExpansionI2c();
  }
  if (!expansionConnected) return;
  if (now - lastExpansionPollMs < cfg::kExpansionPollMs) return;
  lastExpansionPollMs = now;
  for (uint8_t i = 0; i < expansionMotorCount; ++i) {
    if (!expansionReadState(i)) {
      expansionConnected = false;
      break;
    }
  }
}

uint8_t activeMotorCount() {
  if (!expansionEnabled) return cfg::kBaseMotors;
  const uint8_t clamped = expansionMotorCount > cfg::kExpansionMaxMotors ? cfg::kExpansionMaxMotors : expansionMotorCount;
  return cfg::kBaseMotors + clamped;
}

bool isValidMotorId(uint8_t motorId) {
  return motorId < activeMotorCount();
}

pump::PumpController& controllerById(uint8_t motorId) {
  return controllers[motorId];
}

uint8_t readMotorIdFromJson(const DynamicJsonDocument& in, bool* ok = nullptr) {
  if (!in["motorId"].is<int>()) {
    if (ok) *ok = true;
    return 0;
  }
  const int motorId = in["motorId"].as<int>();
  const bool valid = motorId >= 0 && motorId < static_cast<int>(activeMotorCount());
  if (ok) *ok = valid;
  return valid ? static_cast<uint8_t>(motorId) : 0;
}

uint8_t readMotorIdFromRequest(bool* ok = nullptr) {
  if (!server.hasArg("motorId")) {
    const bool valid = selectedMotorId < activeMotorCount();
    if (ok) *ok = valid;
    return valid ? selectedMotorId : 0;
  }
  const int motorId = server.arg("motorId").toInt();
  const bool valid = motorId >= 0 && motorId < static_cast<int>(activeMotorCount());
  if (ok) *ok = valid;
  return valid ? static_cast<uint8_t>(motorId) : 0;
}

void setDriverFrequencyHz(float freqHz) {
  if (freqHz < 1.0f) {
    ledcWriteTone(cfg::kLedcChannel, 0);
    digitalWrite(cfg::kPinStep, LOW);
    digitalWrite(cfg::kPinEnable, HIGH);
    return;
  }

  digitalWrite(cfg::kPinEnable, LOW);
  ledcWriteTone(cfg::kLedcChannel, freqHz);
}

float speedToFrequency(float speed) {
  return fabsf(speed) * kStepsPerRevolution / 60.0f;
}

void applyMotorSpeed(float speed) {
  const auto& conf = controllerById(0).config();
  if (fabsf(speed) < conf.minSpeed) {
    setDriverFrequencyHz(0);
    return;
  }

  digitalWrite(cfg::kPinDir, speed >= 0 ? LOW : HIGH);
  setDriverFrequencyHz(speedToFrequency(speed));
}

void writeMotorState(JsonObject out, const pump::PumpController& ctrl, uint8_t motorId) {
  const auto& st = ctrl.state();
  const char* modeName = st.mode == pump::Mode::DOSING ? "dosing" : "flow_lph";
  const float flowMlMin = st.currentSpeed * ((st.currentSpeed >= 0) ? st.mlPerRevCw : st.mlPerRevCcw);
  const float targetFlowMlMin = st.targetSpeed * ((st.targetSpeed >= 0) ? st.mlPerRevCw : st.mlPerRevCcw);
  out["motorId"] = motorId;
  out["alias"] = motorAliases[motorId];
  out["mode"] = static_cast<uint8_t>(st.mode);
  out["modeName"] = modeName;
  out["running"] = st.running;
  out["flowMlMin"] = flowMlMin;
  out["flowLph"] = flowMlMin * 0.06f;
  out["targetFlowMlMin"] = targetFlowMlMin;
  out["targetFlowLph"] = targetFlowMlMin * 0.06f;
  out["dosingFlowLph"] = fabsf(st.dosingSpeed * st.mlPerRevCw * 0.06f);
  out["direction"] = (st.targetSpeed >= 0) ? "forward" : "reverse";
  out["preferredReverse"] = preferredReverse[motorId];
  out["mlPerRevCw"] = st.mlPerRevCw;
  out["mlPerRevCcw"] = st.mlPerRevCcw;
  out["dosingRemainingMl"] = st.dosingRemainingMl;
  out["uptimeSec"] = st.totalMotorUptimeSec;
  out["totalPumpedL"] = st.totalPumpedVolumeL;
  out["totalHoseL"] = st.totalHoseVolumeL;
}

void savePersistentState() {
  for (uint8_t i = 0; i < cfg::kMaxMotors; ++i) {
    const auto& st = controllerById(i).state();
    char key[24];
    snprintf(key, sizeof(key), "last_speed_%u", i);
    prefs.putFloat(key, st.lastManualSpeed);
    snprintf(key, sizeof(key), "max_speed_%u", i);
    prefs.putFloat(key, controllerById(i).config().maxSpeed);
    snprintf(key, sizeof(key), "ml_cw_%u", i);
    prefs.putFloat(key, st.mlPerRevCw);
    snprintf(key, sizeof(key), "ml_ccw_%u", i);
    prefs.putFloat(key, st.mlPerRevCcw);
    snprintf(key, sizeof(key), "dose_speed_%u", i);
    prefs.putFloat(key, st.dosingSpeed);
    snprintf(key, sizeof(key), "pref_rev_%u", i);
    prefs.putBool(key, preferredReverse[i]);
    snprintf(key, sizeof(key), "uptime_%u", i);
    prefs.putULong(key, st.totalMotorUptimeSec);
    snprintf(key, sizeof(key), "vol_total_%u", i);
    prefs.putDouble(key, st.totalPumpedVolumeL);
    snprintf(key, sizeof(key), "vol_hose_%u", i);
    prefs.putDouble(key, st.totalHoseVolumeL);
    snprintf(key, sizeof(key), "alias_%u", i);
    prefs.putString(key, motorAliases[i]);
  }
  prefs.putBool("exp_en", expansionEnabled);
  prefs.putUChar("exp_count", expansionMotorCount);
  prefs.putString("exp_if", expansionInterface);
  prefs.putUChar("sel_motor", selectedMotorId);
  prefs.putBool("auth_en", webAuthEnabled);
  prefs.putString("auth_user", webAuthUser);
  prefs.putString("auth_pass", webAuthPass);
  prefs.putString("ntp_server", ntpServer);
  prefs.putString("ui_lang", uiLanguage);
  prefs.putBool("grow_tab_en", growthProgramEnabled);
  prefs.putBool("ph_reg_en", phRegulationEnabled);
  prefs.putString("fw_repo", firmwareRepo);
  prefs.putString("fw_asset", firmwareAssetName);
  prefs.putString("fw_fs_asset", firmwareFsAssetName);
  prefs.putInt("tz_offset_min", tzOffsetMinutes);
  DynamicJsonDocument schedDoc(2048);
  JsonArray arr = schedDoc.createNestedArray("entries");
  for (uint8_t i = 0; i < cfg::kMaxSchedules; ++i) {
    JsonObject e = arr.createNestedObject();
    e["enabled"] = doseSchedules[i].enabled;
    e["hour"] = doseSchedules[i].hour;
    e["minute"] = doseSchedules[i].minute;
    e["volumeMl"] = doseSchedules[i].volumeMl;
    e["reverse"] = doseSchedules[i].reverse;
    e["motorId"] = doseSchedules[i].motorId;
    e["name"] = doseSchedules[i].name;
    e["weekdaysMask"] = doseSchedules[i].weekdaysMask;
  }
  String schedJson;
  serializeJson(schedDoc, schedJson);
  prefs.putString("dose_sched", schedJson);
}

void loadPersistentState() {
  for (uint8_t i = 0; i < cfg::kMaxMotors; ++i) {
    auto& ctrl = controllerById(i);
    auto& st = ctrl.mutableState();
    char key[24];
    snprintf(key, sizeof(key), "last_speed_%u", i);
    st.lastManualSpeed = prefs.getFloat(key, i == 0 ? prefs.getFloat("last_speed", 120.0f) : 120.0f);
    snprintf(key, sizeof(key), "max_speed_%u", i);
    ctrl.setMaxSpeed(prefs.getFloat(key, i == 0 ? prefs.getFloat("max_speed", ctrl.config().maxSpeed) : ctrl.config().maxSpeed));
    snprintf(key, sizeof(key), "ml_cw_%u", i);
    st.mlPerRevCw = prefs.getFloat(key, i == 0 ? prefs.getFloat("ml_cw", st.mlPerRevCw) : st.mlPerRevCw);
    snprintf(key, sizeof(key), "ml_ccw_%u", i);
    st.mlPerRevCcw = prefs.getFloat(key, i == 0 ? prefs.getFloat("ml_ccw", st.mlPerRevCcw) : st.mlPerRevCcw);
    snprintf(key, sizeof(key), "dose_speed_%u", i);
    st.dosingSpeed = prefs.getFloat(key, i == 0 ? prefs.getFloat("dosing_speed", st.dosingSpeed) : st.dosingSpeed);
    snprintf(key, sizeof(key), "pref_rev_%u", i);
    preferredReverse[i] = prefs.getBool(key, i == 0 ? prefs.getBool("pref_rev", false) : false);
    snprintf(key, sizeof(key), "uptime_%u", i);
    st.totalMotorUptimeSec = prefs.getULong(key, i == 0 ? prefs.getULong("uptime", 0) : 0);
    snprintf(key, sizeof(key), "vol_total_%u", i);
    st.totalPumpedVolumeL = prefs.getDouble(key, i == 0 ? prefs.getDouble("vol_total", 0.0) : 0.0);
    snprintf(key, sizeof(key), "vol_hose_%u", i);
    st.totalHoseVolumeL = prefs.getDouble(key, i == 0 ? prefs.getDouble("vol_hose", 0.0) : 0.0);
    snprintf(key, sizeof(key), "alias_%u", i);
    const String legacyAlias = (i == 0) ? prefs.getString("motor_alias", defaultMotorAlias(i)) : defaultMotorAlias(i);
    motorAliases[i] = normalizedMotorAlias(prefs.getString(key, legacyAlias), i);
  }
  expansionEnabled = prefs.getBool("exp_en", false);
  expansionMotorCount = prefs.getUChar("exp_count", 0);
  if (expansionMotorCount > cfg::kExpansionMaxMotors) expansionMotorCount = cfg::kExpansionMaxMotors;
  expansionInterface = prefs.getString("exp_if", "i2c");
  if (expansionInterface != "i2c" && expansionInterface != "rs485" && expansionInterface != "uart") {
    expansionInterface = "i2c";
  }
  selectedMotorId = prefs.getUChar("sel_motor", 0);
  if (!isValidMotorId(selectedMotorId)) selectedMotorId = 0;
  webAuthEnabled = prefs.getBool("auth_en", false);
  webAuthUser = prefs.getString("auth_user", "admin");
  webAuthPass = prefs.getString("auth_pass", "admin");
  ntpServer = prefs.getString("ntp_server", "time.google.com");
  uiLanguage = prefs.getString("ui_lang", "en");
  growthProgramEnabled = prefs.getBool("grow_tab_en", false);
  phRegulationEnabled = prefs.getBool("ph_reg_en", false);
  firmwareRepo = prefs.getString("fw_repo", cfg::kDefaultFirmwareRepo);
  firmwareAssetName = prefs.getString("fw_asset", cfg::kDefaultFirmwareAsset);
  firmwareFsAssetName = prefs.getString("fw_fs_asset", cfg::kDefaultFirmwareFsAsset);
  if (webAuthUser.length() == 0) webAuthUser = "admin";
  if (webAuthPass.length() == 0) webAuthPass = "admin";
  if (ntpServer.length() == 0) ntpServer = "time.google.com";
  if (firmwareRepo.length() == 0) firmwareRepo = cfg::kDefaultFirmwareRepo;
  if (firmwareAssetName.length() == 0) firmwareAssetName = cfg::kDefaultFirmwareAsset;
  if (firmwareFsAssetName.length() == 0) firmwareFsAssetName = cfg::kDefaultFirmwareFsAsset;
  if (uiLanguage != "ru" && uiLanguage != "en") uiLanguage = "en";
  tzOffsetMinutes = prefs.getInt("tz_offset_min", 0);
  String schedJson = prefs.getString("dose_sched", "");
  if (schedJson.length() > 0) {
    DynamicJsonDocument schedDoc(2048);
    if (deserializeJson(schedDoc, schedJson) == DeserializationError::Ok) {
      JsonArray arr = schedDoc["entries"].as<JsonArray>();
      uint8_t i = 0;
      for (JsonObject e : arr) {
        if (i >= cfg::kMaxSchedules) break;
        doseSchedules[i].enabled = e["enabled"] | false;
        doseSchedules[i].hour = e["hour"] | 0;
        doseSchedules[i].minute = e["minute"] | 0;
        doseSchedules[i].volumeMl = e["volumeMl"] | 0;
        doseSchedules[i].reverse = e["reverse"] | false;
        doseSchedules[i].motorId = e["motorId"] | 0;
        if (!isValidMotorId(doseSchedules[i].motorId)) doseSchedules[i].motorId = 0;
        setScheduleName(doseSchedules[i], e["name"].is<const char*>() ? e["name"].as<String>() : String(""));
        doseSchedules[i].weekdaysMask = e["weekdaysMask"] | 0x7F;
        doseSchedules[i].lastRunYDay = -1;
        ++i;
      }
    }
  }
}

void writeJsonState(DynamicJsonDocument& doc, uint8_t motorId) {
  if (!isValidMotorId(motorId)) motorId = 0;
  const auto& st = controllerById(motorId).state();
  struct tm nowTm{};
  const char* modeName = st.mode == pump::Mode::DOSING ? "dosing" : "flow_lph";
  doc["firmware"] = cfg::kFirmwareVersion;
  doc["motorId"] = motorId;
  doc["motorAlias"] = motorAliases[motorId];
  doc["selectedMotorId"] = selectedMotorId;
  doc["activeMotorCount"] = activeMotorCount();
  JsonObject expansion = doc.createNestedObject("expansion");
  expansion["enabled"] = expansionEnabled;
  expansion["interface"] = expansionInterface;
  expansion["motorCount"] = expansionMotorCount;
  expansion["connected"] = expansionConnected;
  expansion["address"] = expansionConnected ? expansionI2cAddress : 0;
  doc["mode"] = static_cast<uint8_t>(st.mode);
  doc["modeName"] = modeName;
  doc["running"] = st.running;
  const float flowMlMin = st.currentSpeed * ((st.currentSpeed >= 0) ? st.mlPerRevCw : st.mlPerRevCcw);
  const float targetFlowMlMin = st.targetSpeed * ((st.targetSpeed >= 0) ? st.mlPerRevCw : st.mlPerRevCcw);
  doc["flowMlMin"] = flowMlMin;
  doc["flowLph"] = flowMlMin * 0.06f;
  doc["targetFlowMlMin"] = targetFlowMlMin;
  doc["targetFlowLph"] = targetFlowMlMin * 0.06f;
  doc["dosingFlowLph"] = fabsf(st.dosingSpeed * st.mlPerRevCw * 0.06f);
  doc["direction"] = (st.targetSpeed >= 0) ? "forward" : "reverse";
  doc["preferredReverse"] = preferredReverse[motorId];
  doc["uiLanguage"] = uiLanguage;
  doc["mlPerRevCw"] = st.mlPerRevCw;
  doc["mlPerRevCcw"] = st.mlPerRevCcw;
  doc["dosingRemainingMl"] = st.dosingRemainingMl;
  doc["uptimeSec"] = st.totalMotorUptimeSec;
  doc["totalPumpedL"] = st.totalPumpedVolumeL;
  doc["totalHoseL"] = st.totalHoseVolumeL;
  doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  if (getLocalTimeWithOffset(&nowTm)) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", nowTm.tm_hour, nowTm.tm_min, nowTm.tm_sec);
    doc["time"] = buf;
  } else {
    doc["time"] = "-";
  }

  JsonArray motors = doc.createNestedArray("motors");
  for (uint8_t i = 0; i < activeMotorCount(); ++i) {
    JsonObject motor = motors.createNestedObject();
    writeMotorState(motor, controllerById(i), i);
  }
}

void sendJson(int code, DynamicJsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.send(code, "application/json", out);
}

bool parseBody(DynamicJsonDocument& doc) {
  if (!server.hasArg("plain")) return false;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  return !err;
}

bool ensureAuthenticated() {
  if (!webAuthEnabled) return true;
  if (server.authenticate(webAuthUser.c_str(), webAuthPass.c_str())) return true;
  server.requestAuthentication(BASIC_AUTH, "Peristaltic Pump");
  return false;
}

bool startDosingNow(uint8_t motorId, uint16_t volumeMl, bool reverse) {
  if (!isValidMotorId(motorId)) return false;
  if (volumeMl == 0) return false;
  auto& ctrl = controllerById(motorId);
  if (ctrl.state().running) return false;
  preferredReverse[motorId] = reverse;
  char key[24];
  snprintf(key, sizeof(key), "pref_rev_%u", motorId);
  prefs.putBool(key, preferredReverse[motorId]);
  if (motorId == 0) {
    ctrl.startDosing(reverse ? -static_cast<int32_t>(volumeMl) : static_cast<int32_t>(volumeMl));
    return true;
  }
  const uint8_t remoteIdx = motorId - 1;
  if (!expansionStartDosing(remoteIdx, volumeMl, reverse)) return false;
  delay(2);
  return expansionReadState(remoteIdx);
}

bool getLocalTimeWithOffset(struct tm* outTm) {
  const time_t now = time(nullptr);
  if (now < 100000) return false;
  const time_t shifted = now + static_cast<time_t>(tzOffsetMinutes) * 60;
  gmtime_r(&shifted, outTm);
  return true;
}

void applyNtpConfig() {
  configTime(0, 0, ntpServer.c_str());
}

bool isWeekdayEnabled(uint8_t weekdaysMask, int tmWday) {
  if (tmWday < 0 || tmWday > 6) return false;
  const uint8_t bit = (tmWday == 0) ? 6 : static_cast<uint8_t>(tmWday - 1);  // Sun=>6, Mon=>0
  return (weekdaysMask & (1u << bit)) != 0;
}

void processDosingSchedule() {
  struct tm nowTm{};
  if (!getLocalTimeWithOffset(&nowTm)) return;

  for (uint8_t i = 0; i < cfg::kMaxSchedules; ++i) {
    auto& s = doseSchedules[i];
    if (!s.enabled || s.volumeMl == 0) continue;
    if (s.hour > 23 || s.minute > 59) continue;
    if (!isWeekdayEnabled(s.weekdaysMask, nowTm.tm_wday)) continue;
    if (nowTm.tm_hour != s.hour || nowTm.tm_min != s.minute) continue;
    if (s.lastRunYDay == nowTm.tm_yday) continue;
    if (!isValidMotorId(s.motorId)) continue;

    if (controllerById(s.motorId).state().running) {
      // Busy at trigger minute: skip this schedule for today.
      s.lastRunYDay = nowTm.tm_yday;
      continue;
    }
    if (startDosingNow(s.motorId, s.volumeMl, s.reverse)) {
      s.lastRunYDay = nowTm.tm_yday;
      break;
    }
  }
}

void handleOptions() {
  DynamicJsonDocument doc(64);
  doc["ok"] = true;
  sendJson(200, doc);
}

void setupApi() {
  server.on("/", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    File f = LittleFS.open("/index.html", "r");
    if (!f) {
      server.send(500, "text/plain", "index.html not found in LittleFS");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
  });
  server.on("/styles.css", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    File f = LittleFS.open("/styles.css", "r");
    if (!f) {
      server.send(404, "text/plain", "styles.css not found");
      return;
    }
    server.streamFile(f, "text/css");
    f.close();
  });
  server.on("/app.js", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    File f = LittleFS.open("/app.js", "r");
    if (!f) {
      server.send(404, "text/plain", "app.js not found");
      return;
    }
    server.streamFile(f, "application/javascript");
    f.close();
  });
  server.on("/growth-schedule.js", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    File f = LittleFS.open("/growth-schedule.js", "r");
    if (!f) {
      server.send(404, "text/plain", "growth-schedule.js not found");
      return;
    }
    server.streamFile(f, "application/javascript");
    f.close();
  });

  server.on("/api/state", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    bool ok = false;
    const uint8_t motorId = readMotorIdFromRequest(&ok);
    if (!ok) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/start", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(256);
    if (server.hasArg("plain") && !parseBody(in)) in.clear();
    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    if (motorId == 0) {
      controllerById(motorId).start();
    } else if (!expansionStart(motorId - 1) || !expansionReadState(motorId - 1)) {
      DynamicJsonDocument err(128);
      err["error"] = "expansion motor start failed";
      sendJson(503, err);
      return;
    }
    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/stop", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(256);
    if (server.hasArg("plain") && !parseBody(in)) in.clear();
    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    if (motorId == 0) {
      controllerById(motorId).stop(false);
    } else if (!expansionStop(motorId - 1) || !expansionReadState(motorId - 1)) {
      DynamicJsonDocument err(128);
      err["error"] = "expansion motor stop failed";
      sendJson(503, err);
      return;
    }
    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/ui/preferences", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(512);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }
    bool hasUpdate = false;
    if (in["reverse"].is<bool>()) {
      preferredReverse[selectedMotorId] = in["reverse"].as<bool>();
      char key[24];
      snprintf(key, sizeof(key), "pref_rev_%u", selectedMotorId);
      prefs.putBool(key, preferredReverse[selectedMotorId]);
      hasUpdate = true;
    }
    if (in["motorId"].is<int>()) {
      const int motorId = in["motorId"].as<int>();
      if (motorId < 0 || motorId >= static_cast<int>(activeMotorCount())) {
        DynamicJsonDocument err(128);
        err["error"] = "invalid motorId";
        sendJson(400, err);
        return;
      }
      selectedMotorId = static_cast<uint8_t>(motorId);
      hasUpdate = true;
    }
    if (in["language"].is<const char*>()) {
      const String lang = in["language"].as<String>();
      if (lang != "en" && lang != "ru") {
        DynamicJsonDocument err(128);
        err["error"] = "language must be en or ru";
        sendJson(400, err);
        return;
      }
      uiLanguage = lang;
      hasUpdate = true;
    }
    if (!hasUpdate) {
      DynamicJsonDocument err(128);
      err["error"] = "at least one field is required: reverse, motorId or language";
      sendJson(400, err);
      return;
    }
    prefs.putUChar("sel_motor", selectedMotorId);
    prefs.putString("ui_lang", uiLanguage);
    DynamicJsonDocument doc(2048);
    writeJsonState(doc, selectedMotorId);
    sendJson(200, doc);
  });

  server.on("/api/ui/preferences", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument doc(1024);
    doc["reverse"] = preferredReverse[selectedMotorId];
    doc["motorId"] = selectedMotorId;
    doc["activeMotorCount"] = activeMotorCount();
    doc["language"] = uiLanguage;
    JsonArray reverses = doc.createNestedArray("reverseByMotor");
    for (uint8_t i = 0; i < activeMotorCount(); ++i) {
      reverses.add(preferredReverse[i]);
    }
    sendJson(200, doc);
  });

  server.on("/api/flow", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(1024);
    if (!parseBody(in) || !in["litersPerHour"].is<float>()) {
      DynamicJsonDocument err(256);
      err["error"] = "litersPerHour is required";
      sendJson(400, err);
      return;
    }

    const float lph = in["litersPerHour"].as<float>();
    const bool reverse = in["reverse"] | false;
    const String direction = (in["direction"].is<const char*>()) ? in["direction"].as<String>() : String("");
    if (lph < 0) {
      DynamicJsonDocument err(256);
      err["error"] = "litersPerHour must be >= 0";
      sendJson(400, err);
      return;
    }
    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(256);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    const bool useReverse = reverse || direction == "ccw" || direction == "reverse";
    preferredReverse[motorId] = useReverse;
    char key[24];
    snprintf(key, sizeof(key), "pref_rev_%u", motorId);
    prefs.putBool(key, preferredReverse[motorId]);
    if (motorId == 0) {
      const auto& st = controllerById(motorId).state();
      const float mlPerRev = useReverse ? st.mlPerRevCcw : st.mlPerRevCw;
      const float speed = (lph * 1000.0f / 60.0f) / mlPerRev * (useReverse ? -1.0f : 1.0f);
      controllerById(motorId).setSpeed(speed, pump::Mode::FLOW);
    } else if (!expansionSetFlow(motorId - 1, lph, useReverse) || !expansionReadState(motorId - 1)) {
      DynamicJsonDocument err(256);
      err["error"] = "expansion flow command failed";
      sendJson(503, err);
      return;
    }

    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  auto dosingHandler = []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(1024);
    if (!parseBody(in) || !in["volumeMl"].is<int>()) {
      DynamicJsonDocument err(256);
      err["error"] = "volumeMl is required";
      sendJson(400, err);
      return;
    }

    const int32_t volume = in["volumeMl"].as<int32_t>();
    if (volume <= 0) {
      DynamicJsonDocument err(256);
      err["error"] = "volumeMl must be > 0; use reverse=true";
      sendJson(400, err);
      return;
    }
    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(256);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    const bool reverse = in["reverse"] | false;
    preferredReverse[motorId] = reverse;
    char key[24];
    snprintf(key, sizeof(key), "pref_rev_%u", motorId);
    prefs.putBool(key, preferredReverse[motorId]);
    if (motorId == 0) {
      controllerById(motorId).startDosing(reverse ? -volume : volume);
    } else if (!expansionStartDosing(motorId - 1, static_cast<uint16_t>(volume), reverse) || !expansionReadState(motorId - 1)) {
      DynamicJsonDocument err(256);
      err["error"] = "expansion dosing command failed";
      sendJson(503, err);
      return;
    }
    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  };
  server.on("/api/dosing", HTTP_POST, dosingHandler);

  server.on("/api/settings", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    bool ok = false;
    const uint8_t motorId = readMotorIdFromRequest(&ok);
    if (!ok) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    DynamicJsonDocument doc(1024);
    const auto& ctrl = controllerById(motorId);
    const auto& st = ctrl.state();
    doc["motorId"] = motorId;
    doc["motorAlias"] = motorAliases[motorId];
    doc["mlPerRevCw"] = st.mlPerRevCw;
    doc["mlPerRevCcw"] = st.mlPerRevCcw;
    doc["dosingFlowLph"] = fabsf(st.dosingSpeed * st.mlPerRevCw * 0.06f);
    doc["maxFlowLph"] = ctrl.config().maxSpeed * st.mlPerRevCw * 0.06f;
    doc["ntpServer"] = ntpServer;
    doc["tzOffsetMinutes"] = tzOffsetMinutes;
    doc["growthProgramEnabled"] = growthProgramEnabled;
    doc["phRegulationEnabled"] = phRegulationEnabled;
    JsonObject firmware = doc.createNestedObject("firmwareUpdate");
    firmware["repo"] = firmwareRepo;
    firmware["assetName"] = firmwareAssetName;
    firmware["filesystemAssetName"] = firmwareFsAssetName;
    firmware["currentVersion"] = cfg::kFirmwareVersion;
    JsonArray aliases = doc.createNestedArray("motorAliases");
    for (uint8_t i = 0; i < activeMotorCount(); ++i) aliases.add(motorAliases[i]);
    JsonObject expansion = doc.createNestedObject("expansion");
    expansion["enabled"] = expansionEnabled;
    expansion["interface"] = expansionInterface;
    expansion["motorCount"] = expansionMotorCount;
    expansion["connected"] = expansionConnected;
    expansion["address"] = expansionConnected ? expansionI2cAddress : 0;
    sendJson(200, doc);
  });

  server.on("/api/settings", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(1024);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }

    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    auto& ctrl = controllerById(motorId);
    const auto& st = ctrl.state();
    const float cw = in["mlPerRevCw"] | st.mlPerRevCw;
    const float ccw = in["mlPerRevCcw"] | st.mlPerRevCcw;
    float dosingSpeed = st.dosingSpeed;
    float dosingFlowLph = -1.0f;
    float maxFlowLph = ctrl.config().maxSpeed * st.mlPerRevCw * 0.06f;
    if (in["dosingFlowLph"].is<float>()) dosingFlowLph = in["dosingFlowLph"].as<float>();
    if (in["maxFlowLph"].is<float>()) maxFlowLph = in["maxFlowLph"].as<float>();
    if (dosingFlowLph > 0.0f) {
      const float mlPerRev = cw > 0.0f ? cw : st.mlPerRevCw;
      dosingSpeed = (dosingFlowLph * 1000.0f / 60.0f) / mlPerRev;
    }
    if (maxFlowLph <= 0.0f) {
      DynamicJsonDocument err(128);
      err["error"] = "maxFlowLph must be > 0";
      sendJson(400, err);
      return;
    }
    if (in["ntpServer"].is<const char*>()) {
      const String candidate = in["ntpServer"].as<String>();
      if (candidate.length() == 0) {
        DynamicJsonDocument err(128);
        err["error"] = "ntpServer cannot be empty";
        sendJson(400, err);
        return;
      }
      ntpServer = candidate;
      applyNtpConfig();
    }
    if (in["tzOffsetMinutes"].is<int>()) {
      const int candidateTzOffset = in["tzOffsetMinutes"].as<int>();
      if (candidateTzOffset < -720 || candidateTzOffset > 840) {
        DynamicJsonDocument err(128);
        err["error"] = "tzOffsetMinutes must be between -720 and 840";
        sendJson(400, err);
        return;
      }
      tzOffsetMinutes = candidateTzOffset;
    }
    if (in["motorAliases"].is<JsonArray>()) {
      JsonArray aliases = in["motorAliases"].as<JsonArray>();
      uint8_t i = 0;
      for (JsonVariant v : aliases) {
        if (i >= cfg::kMaxMotors) break;
        if (v.is<const char*>()) {
          motorAliases[i] = normalizedMotorAlias(v.as<String>(), i);
        }
        ++i;
      }
    }
    if (in["growthProgramEnabled"].is<bool>()) {
      growthProgramEnabled = in["growthProgramEnabled"].as<bool>();
    }
    if (in["phRegulationEnabled"].is<bool>()) {
      phRegulationEnabled = in["phRegulationEnabled"].as<bool>();
    }
    if (in["expansion"].is<JsonObject>()) {
      JsonObject exp = in["expansion"];
      if (exp["enabled"].is<bool>()) expansionEnabled = exp["enabled"].as<bool>();
      if (exp["motorCount"].is<int>()) {
        int count = exp["motorCount"].as<int>();
        if (count < 0) count = 0;
        if (count > cfg::kExpansionMaxMotors) count = cfg::kExpansionMaxMotors;
        expansionMotorCount = static_cast<uint8_t>(count);
      }
      if (exp["interface"].is<const char*>()) {
        const String iface = exp["interface"].as<String>();
        if (iface == "i2c" || iface == "rs485" || iface == "uart") {
          expansionInterface = iface;
        }
      }
      if (!isValidMotorId(selectedMotorId)) selectedMotorId = 0;
      if (!expansionEnabled) {
        expansionConnected = false;
      } else if (expansionInterface == "i2c") {
        discoverExpansionI2c();
      }
    }
    if (motorId == 0) {
      const float mlPerRevForMax = cw > 0.0f ? cw : st.mlPerRevCw;
      ctrl.setMaxSpeed((maxFlowLph * 1000.0f / 60.0f) / mlPerRevForMax);
      ctrl.setMlPerRev(cw, ccw);
      ctrl.setDosingSpeed(dosingSpeed);
    } else {
      if (!expansionSetSettings(motorId - 1, cw, ccw, dosingFlowLph > 0.0f ? dosingFlowLph : fabsf(dosingSpeed * cw * 0.06f), maxFlowLph)) {
        DynamicJsonDocument err(128);
        err["error"] = "expansion settings update failed";
        sendJson(503, err);
        return;
      }
      if (!expansionReadState(motorId - 1)) {
        DynamicJsonDocument err(128);
        err["error"] = "expansion state refresh failed";
        sendJson(503, err);
        return;
      }
    }
    savePersistentState();

    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/firmware/config", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument doc(384);
    doc["repo"] = firmwareRepo;
    doc["assetName"] = firmwareAssetName;
    doc["filesystemAssetName"] = firmwareFsAssetName;
    doc["currentVersion"] = cfg::kFirmwareVersion;
    sendJson(200, doc);
  });

  server.on("/api/firmware/probe", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    if (!server.hasArg("url")) {
      DynamicJsonDocument err(192);
      err["error"] = "url query arg is required";
      sendJson(400, err);
      return;
    }
    const String url = server.arg("url");
    if (!isValidHttpUrl(url)) {
      DynamicJsonDocument err(224);
      err["error"] = "url must be valid http(s) url";
      sendJson(400, err);
      return;
    }
    int code = 0;
    int contentLength = -1;
    String probeError;
    const bool ok = probeHttpUrl(url, &code, &contentLength, &probeError);
    DynamicJsonDocument doc(320);
    doc["ok"] = ok;
    doc["url"] = url;
    doc["statusCode"] = code;
    if (contentLength >= 0) doc["contentLength"] = contentLength;
    if (!ok && probeError.length() > 0) doc["error"] = probeError;
    sendJson(ok ? 200 : 502, doc);
  });

  server.on("/api/firmware/config", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(384);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }
    if (in["repo"].is<const char*>()) {
      const String candidate = in["repo"].as<String>();
      if (candidate.length() > 0 && !isValidRepoSlug(candidate)) {
        DynamicJsonDocument err(192);
        err["error"] = "repo must be in owner/repo format";
        sendJson(400, err);
        return;
      }
      firmwareRepo = candidate;
      prefs.putString("fw_repo", firmwareRepo);
    }
    if (in["assetName"].is<const char*>()) {
      String candidate = in["assetName"].as<String>();
      candidate.trim();
      if (candidate.length() == 0) candidate = cfg::kDefaultFirmwareAsset;
      firmwareAssetName = candidate;
      prefs.putString("fw_asset", firmwareAssetName);
    }
    if (in["filesystemAssetName"].is<const char*>()) {
      String candidate = in["filesystemAssetName"].as<String>();
      candidate.trim();
      if (candidate.length() == 0) candidate = cfg::kDefaultFirmwareFsAsset;
      firmwareFsAssetName = candidate;
      prefs.putString("fw_fs_asset", firmwareFsAssetName);
    }
    DynamicJsonDocument doc(384);
    doc["repo"] = firmwareRepo;
    doc["assetName"] = firmwareAssetName;
    doc["filesystemAssetName"] = firmwareFsAssetName;
    doc["currentVersion"] = cfg::kFirmwareVersion;
    sendJson(200, doc);
  });

  server.on("/api/firmware/releases", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    String repo = firmwareRepo;
    if (server.hasArg("repo")) repo = server.arg("repo");
    if (!isValidRepoSlug(repo)) {
      DynamicJsonDocument err(192);
      err["error"] = "configure firmware repo in owner/repo format";
      sendJson(400, err);
      return;
    }

    const String endpoint = String("https://api.github.com/repos/") + repo + "/releases?per_page=" + String(cfg::kMaxFirmwareReleases);
    int statusCode = 0;
    String payload;
    if (!githubHttpGet(endpoint, &statusCode, &payload)) {
      DynamicJsonDocument err(192);
      err["error"] = "github request failed";
      sendJson(502, err);
      return;
    }
    if (statusCode != 200) {
      DynamicJsonDocument err(256);
      err["error"] = "github release list request failed";
      err["statusCode"] = statusCode;
      sendJson(502, err);
      return;
    }

    DynamicJsonDocument ghDoc(cfg::kFirmwareReleasesDocBytes);
    if (deserializeJson(ghDoc, payload) != DeserializationError::Ok || !ghDoc.is<JsonArray>()) {
      DynamicJsonDocument err(192);
      err["error"] = "invalid github release list response";
      sendJson(502, err);
      return;
    }

    DynamicJsonDocument doc(4096);
    doc["repo"] = repo;
    doc["assetName"] = firmwareAssetName;
    doc["filesystemAssetName"] = firmwareFsAssetName;
    doc["currentVersion"] = cfg::kFirmwareVersion;
    JsonArray outReleases = doc.createNestedArray("releases");
    for (JsonObjectConst rel : ghDoc.as<JsonArrayConst>()) {
      JsonObject outRel = outReleases.createNestedObject();
      outRel["tag"] = rel["tag_name"] | "";
      outRel["name"] = rel["name"] | "";
      outRel["publishedAt"] = rel["published_at"] | "";
      outRel["draft"] = rel["draft"] | false;
      outRel["prerelease"] = rel["prerelease"] | false;
      JsonArray outAssets = outRel.createNestedArray("assets");
      if (rel["assets"].is<JsonArrayConst>()) {
        for (JsonObjectConst asset : rel["assets"].as<JsonArrayConst>()) {
          JsonObject outAsset = outAssets.createNestedObject();
          outAsset["name"] = asset["name"] | "";
          outAsset["size"] = asset["size"] | 0;
          outAsset["downloadUrl"] = asset["browser_download_url"] | "";
        }
      }
    }
    sendJson(200, doc);
  });

  server.on("/api/firmware/update", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(1024);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }
    if (WiFi.status() != WL_CONNECTED) {
      DynamicJsonDocument err(128);
      err["error"] = "wifi is not connected";
      sendJson(503, err);
      return;
    }

    String repo = firmwareRepo;
    String assetName = firmwareAssetName;
    String fsAssetName = firmwareFsAssetName;
    String mode = "latest";
    String tag = "";
    if (in["repo"].is<const char*>()) repo = in["repo"].as<String>();
    if (in["assetName"].is<const char*>()) assetName = in["assetName"].as<String>();
    if (in["filesystemAssetName"].is<const char*>()) fsAssetName = in["filesystemAssetName"].as<String>();
    if (in["mode"].is<const char*>()) mode = in["mode"].as<String>();
    if (in["tag"].is<const char*>()) tag = in["tag"].as<String>();
    if (assetName.length() == 0) assetName = cfg::kDefaultFirmwareAsset;
    if (fsAssetName.length() == 0) fsAssetName = cfg::kDefaultFirmwareFsAsset;

    String resolvedTag;
    String resolvedAssetName;
    String resolvedFsAssetName;
    String assetUrl;
    String fsUrl;
    if (mode != "latest" && mode != "tag" && mode != "url") {
      DynamicJsonDocument err(160);
      err["error"] = "mode must be latest, tag or url";
      sendJson(400, err);
      return;
    }
    if (mode == "url") {
      assetUrl = in["url"] | "";
      fsUrl = in["filesystemUrl"] | in["fsUrl"] | "";
      if (!isValidHttpUrl(assetUrl) || !isValidHttpUrl(fsUrl)) {
        DynamicJsonDocument err(256);
        err["error"] = "url and filesystemUrl must be valid http(s) urls when mode=url";
        sendJson(400, err);
        return;
      }
      resolvedTag = "local";
      resolvedAssetName = assetName;
      resolvedFsAssetName = fsAssetName;
    } else {
      String resolveError;
      int githubStatus = 0;
      if (!resolveReleaseForUpdate(repo, mode, tag, assetName, fsAssetName,
                                   &resolvedTag, &resolvedAssetName, &assetUrl,
                                   &resolvedFsAssetName, &fsUrl, &resolveError, &githubStatus)) {
        DynamicJsonDocument err(320);
        err["error"] = resolveError;
        if (githubStatus > 0) err["statusCode"] = githubStatus;
        sendJson(400, err);
        return;
      }
    }

    int fsUpdateError = 0;
    String fsUpdateErrorString;
    if (!performHttpOta(fsUrl, U_SPIFFS, &fsUpdateError, &fsUpdateErrorString)) {
      DynamicJsonDocument err(384);
      err["error"] = "filesystem update failed";
      err["updateError"] = fsUpdateError;
      err["updateErrorString"] = fsUpdateErrorString;
      sendJson(500, err);
      return;
    }

    int fwUpdateError = 0;
    String fwUpdateErrorString;
    if (performHttpOta(assetUrl, U_FLASH, &fwUpdateError, &fwUpdateErrorString)) {
      DynamicJsonDocument ok(512);
      ok["ok"] = true;
      ok["repo"] = repo;
      ok["tag"] = resolvedTag;
      ok["assetName"] = resolvedAssetName;
      ok["filesystemAssetName"] = resolvedFsAssetName;
      ok["filesystemUrl"] = fsUrl;
      ok["url"] = assetUrl;
      ok["message"] = "firmware and filesystem updated, restarting";
      sendJson(200, ok);
      delay(500);
      ESP.restart();
      return;
    }
    DynamicJsonDocument err(384);
    err["error"] = "firmware update failed";
    err["updateError"] = fwUpdateError;
    err["updateErrorString"] = fwUpdateErrorString;
    if (fwUpdateError == 9) {
      err["hint"] = "firmware image may be incompatible with device target (esp32 vs esp32-s3)";
    }
    sendJson(500, err);
  });

  server.on("/api/ui/security", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument doc(256);
    doc["enabled"] = webAuthEnabled;
    doc["username"] = webAuthUser;
    sendJson(200, doc);
  });

  server.on("/api/schedule", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument doc(2048);
    doc["tzOffsetMinutes"] = tzOffsetMinutes;
    JsonArray entries = doc.createNestedArray("entries");
    for (uint8_t i = 0; i < cfg::kMaxSchedules; ++i) {
      if (doseSchedules[i].volumeMl == 0) continue;
      JsonObject e = entries.createNestedObject();
      e["id"] = i;
      e["enabled"] = doseSchedules[i].enabled;
      e["hour"] = doseSchedules[i].hour;
      e["minute"] = doseSchedules[i].minute;
      e["volumeMl"] = doseSchedules[i].volumeMl;
      e["reverse"] = doseSchedules[i].reverse;
      e["motorId"] = doseSchedules[i].motorId;
      e["name"] = doseSchedules[i].name;
      e["weekdaysMask"] = doseSchedules[i].weekdaysMask;
    }
    sendJson(200, doc);
  });

  server.on("/api/schedule", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(4096);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }
    if (in["tzOffsetMinutes"].is<int>()) {
      const int candidateTzOffset = in["tzOffsetMinutes"].as<int>();
      if (candidateTzOffset < -720 || candidateTzOffset > 840) {
        DynamicJsonDocument err(128);
        err["error"] = "tzOffsetMinutes must be between -720 and 840";
        sendJson(400, err);
        return;
      }
      tzOffsetMinutes = candidateTzOffset;
    }
    if (in["entries"].is<JsonArray>()) {
      for (uint8_t i = 0; i < cfg::kMaxSchedules; ++i) {
        doseSchedules[i] = DoseScheduleEntry{};
      }
      uint8_t idx = 0;
      for (JsonObject e : in["entries"].as<JsonArray>()) {
        if (idx >= cfg::kMaxSchedules) break;
        doseSchedules[idx].enabled = e["enabled"] | false;
        doseSchedules[idx].hour = e["hour"] | 0;
        doseSchedules[idx].minute = e["minute"] | 0;
        doseSchedules[idx].volumeMl = e["volumeMl"] | 0;
        doseSchedules[idx].reverse = e["reverse"] | false;
        doseSchedules[idx].motorId = e["motorId"] | 0;
        if (!isValidMotorId(doseSchedules[idx].motorId)) doseSchedules[idx].motorId = 0;
        setScheduleName(doseSchedules[idx], e["name"].is<const char*>() ? e["name"].as<String>() : String(""));
        doseSchedules[idx].weekdaysMask = e["weekdaysMask"] | 0x7F;
        doseSchedules[idx].lastRunYDay = -1;
        ++idx;
      }
    }
    savePersistentState();
    DynamicJsonDocument doc(128);
    doc["ok"] = true;
    sendJson(200, doc);
  });

  server.on("/api/ui/security", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(512);
    if (!parseBody(in)) {
      DynamicJsonDocument err(128);
      err["error"] = "invalid json";
      sendJson(400, err);
      return;
    }
    if (in["enabled"].is<bool>()) {
      webAuthEnabled = in["enabled"].as<bool>();
    }
    if (in["username"].is<const char*>()) {
      String u = in["username"].as<String>();
      if (u.length() == 0) {
        DynamicJsonDocument err(128);
        err["error"] = "username cannot be empty";
        sendJson(400, err);
        return;
      }
      webAuthUser = u;
    }
    if (in["password"].is<const char*>()) {
      String p = in["password"].as<String>();
      if (p.length() == 0) {
        DynamicJsonDocument err(128);
        err["error"] = "password cannot be empty";
        sendJson(400, err);
        return;
      }
      webAuthPass = p;
    }
    prefs.putBool("auth_en", webAuthEnabled);
    prefs.putString("auth_user", webAuthUser);
    prefs.putString("auth_pass", webAuthPass);
    DynamicJsonDocument doc(256);
    doc["enabled"] = webAuthEnabled;
    doc["username"] = webAuthUser;
    sendJson(200, doc);
  });

  server.on("/api/calibration/run", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(512);
    if (!parseBody(in) || !in["direction"].is<const char*>()) {
      DynamicJsonDocument err(256);
      err["error"] = "direction is required (cw/ccw)";
      sendJson(400, err);
      return;
    }

    const int revs = in["revolutions"] | 200;
    if (revs <= 0) {
      DynamicJsonDocument err(256);
      err["error"] = "revolutions must be > 0";
      sendJson(400, err);
      return;
    }

    const String dir = in["direction"].as<String>();
    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(256);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    const auto& st = controllerById(motorId).state();
    const float mlPerRev = (dir == "ccw") ? st.mlPerRevCcw : st.mlPerRevCw;
    const int volumeMl = static_cast<int>(roundf(mlPerRev * revs)) * ((dir == "ccw") ? -1 : 1);
    if (motorId == 0) {
      controllerById(motorId).startDosing(volumeMl);
    } else if (!expansionStartDosing(motorId - 1, static_cast<uint16_t>(abs(volumeMl)), dir == "ccw") || !expansionReadState(motorId - 1)) {
      DynamicJsonDocument err(256);
      err["error"] = "expansion calibration run failed";
      sendJson(503, err);
      return;
    }

    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/calibration/apply", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument in(1024);
    if (!parseBody(in) || !in["direction"].is<const char*>()) {
      DynamicJsonDocument err(256);
      err["error"] = "direction is required (cw/ccw)";
      sendJson(400, err);
      return;
    }
    const float measuredMl = in["measuredMl"] | -1.0f;
    const float revs = in["revolutions"] | -1.0f;
    if (measuredMl <= 0.0f || revs <= 0.0f) {
      DynamicJsonDocument err(256);
      err["error"] = "measuredMl and revolutions must be > 0";
      sendJson(400, err);
      return;
    }

    bool ok = false;
    const uint8_t motorId = readMotorIdFromJson(in, &ok);
    if (!ok) {
      DynamicJsonDocument err(256);
      err["error"] = "invalid motorId";
      sendJson(400, err);
      return;
    }
    auto st = controllerById(motorId).state();
    const float calibrated = measuredMl / revs;
    const String dir = in["direction"].as<String>();
    if (motorId == 0) {
      if (dir == "ccw") {
        controllerById(motorId).setMlPerRev(st.mlPerRevCw, calibrated);
      } else {
        controllerById(motorId).setMlPerRev(calibrated, st.mlPerRevCcw);
      }
    } else {
      const float newCw = (dir == "ccw") ? st.mlPerRevCw : calibrated;
      const float newCcw = (dir == "ccw") ? calibrated : st.mlPerRevCcw;
      const float dosingFlowLph = fabsf(st.dosingSpeed * newCw * 0.06f);
      const float maxFlowLph = controllerById(motorId).config().maxSpeed * newCw * 0.06f;
      if (!expansionSetSettings(motorId - 1, newCw, newCcw, dosingFlowLph, maxFlowLph) || !expansionReadState(motorId - 1)) {
        DynamicJsonDocument err(256);
        err["error"] = "expansion calibration apply failed";
        sendJson(503, err);
        return;
      }
    }
    savePersistentState();

    DynamicJsonDocument doc(2048);
    writeJsonState(doc, motorId);
    sendJson(200, doc);
  });

  server.on("/api/wifi", HTTP_GET, []() {
    if (!ensureAuthenticated()) return;
    DynamicJsonDocument doc(256);
    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["configPortalSsid"] = cfg::kApSsid;
    sendJson(200, doc);
  });

  server.on("/api/wifi/reset", HTTP_POST, []() {
    if (!ensureAuthenticated()) return;
    wifiManager.resetSettings();
    DynamicJsonDocument doc(256);
    doc["ok"] = true;
    doc["message"] = "Wi-Fi settings reset. Device will reboot to AP config portal.";
    sendJson(200, doc);
    delay(300);
    ESP.restart();
  });

  server.onNotFound([]() {
    if (!ensureAuthenticated()) return;
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
      return;
    }

    DynamicJsonDocument doc(256);
    doc["error"] = "not found";
    sendJson(404, doc);
  });

  server.begin();
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  wifiManager.setHostname("peristaltic-esp32");
  wifiManager.setWiFiAutoReconnect(true);

  // Debug path: try known Wi-Fi first so device boots without AP portal.
  WiFi.begin(cfg::kDebugWifiSsid, cfg::kDebugWifiPass);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - started) < cfg::kDebugWifiConnectTimeoutMs) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  wifiManager.setConfigPortalTimeout(cfg::kApPortalTimeoutSec);
  const bool connected = wifiManager.autoConnect(cfg::kApSsid, cfg::kApPass);
  if (!connected) {
    ESP.restart();
  }
}

const char* modeToName(pump::Mode mode) {
  switch (mode) {
    case pump::Mode::FLOW: return "flow_lph";
    case pump::Mode::DOSING: return "dosing";
  }
  return "flow_lph";
}

void drawOledStatus() {
  if (!oledReady) return;
  const uint8_t motorId = isValidMotorId(selectedMotorId) ? selectedMotorId : 0;
  const auto& st = controllerById(motorId).state();
  const float targetFlowMlMin = st.targetSpeed * ((st.targetSpeed >= 0) ? st.mlPerRevCw : st.mlPerRevCcw);
  const float targetFlowLph = fabsf(targetFlowMlMin * 0.06f);
  const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  const bool isRu = (uiLanguage == "ru");
  const char* modeLabel = isRu ? "Rezh:" : "Mode:";
  const char* modeValue = isRu ? ((st.mode == pump::Mode::DOSING) ? "dozirovanie" : "potok") : modeToName(st.mode);
  const char* dirLabel = isRu ? "Napr:" : "Dir: ";
  const char* speedLabel = isRu ? "Skor:" : "SPD: ";
  const char* flowLabel = isRu ? "PotokL/h:" : "Flow L/h:";
  const char* pumpedLabel = isRu ? "NalitoL:" : "Pumped L:";
  const char* dosingLabel = isRu ? "Doza ml:" : "Dosing ml:";

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(modeLabel);
  oled.print(modeValue);
  oled.print(" M");
  oled.print(motorId);
  oled.print(" ");
  oled.println(st.running ? "ON" : "OFF");
  oled.print(dirLabel);
  if (st.targetSpeed >= 0) {
    oled.println(isRu ? "vpered" : "forward");
  } else {
    oled.println(isRu ? "nazad" : "reverse");
  }
  oled.print(speedLabel);
  oled.println(fabsf(st.currentSpeed), 1);
  oled.print(flowLabel);
  oled.println(targetFlowLph, 2);
  oled.print(pumpedLabel);
  oled.println(st.totalPumpedVolumeL, 3);
  oled.print(dosingLabel);
  oled.println(st.dosingRemainingMl, 0);

  if (wifiConnected) {
    oled.drawCircle(121, 5, 1, SSD1306_WHITE);
    oled.drawCircle(121, 5, 3, SSD1306_WHITE);
    oled.drawCircle(121, 5, 5, SSD1306_WHITE);
    oled.fillCircle(121, 5, 1, SSD1306_WHITE);
  }
  oled.display();
}

void setup() {
  Serial.begin(115200);
  prefs.begin("pump", false);
  loadPersistentState();

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  pinMode(cfg::kPinStep, OUTPUT);
  pinMode(cfg::kPinDir, OUTPUT);
  pinMode(cfg::kPinEnable, OUTPUT);
  digitalWrite(cfg::kPinEnable, HIGH);

  ledcSetup(cfg::kLedcChannel, 1, cfg::kLedcResolutionBits);
  ledcAttachPin(cfg::kPinStep, cfg::kLedcChannel);

  Wire.begin(cfg::kPinI2cSda, cfg::kPinI2cScl);
  if (expansionEnabled && expansionInterface == "i2c") {
    discoverExpansionI2c();
  }
  oledReady = oled.begin(SSD1306_SWITCHCAPVCC, cfg::kOledAddr);
  if (oledReady) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("OLED ready");
    oled.print("SDA=");
    oled.println(cfg::kPinI2cSda);
    oled.print("SCL=");
    oled.println(cfg::kPinI2cScl);
    oled.display();
  } else {
    Serial.println("OLED init failed on I2C 0x3C");
  }

  setupWifi();
  applyNtpConfig();
  setupApi();

  lastControlMs = millis();
  lastSaveMs = millis();
  lastOledMs = millis();

  Serial.printf("Pump firmware %s started\n", cfg::kFirmwareVersion);
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
}

void loop() {
  const uint32_t now = millis();
  server.handleClient();
  refreshExpansionState();
  processDosingSchedule();

  if (now - lastControlMs >= cfg::kControlTickMs) {
    const uint32_t delta = now - lastControlMs;
    controllerById(0).tick(delta);
    lastControlMs = now;
    applyMotorSpeed(controllerById(0).state().currentSpeed);
  }

  if (now - lastSaveMs >= cfg::kSavePeriodMs) {
    lastSaveMs = now;
    savePersistentState();
  }

  if (now - lastOledMs >= cfg::kOledRefreshMs) {
    lastOledMs = now;
    drawOledStatus();
  }
}
