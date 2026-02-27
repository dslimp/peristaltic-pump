#include <Arduino.h>
#include <Wire.h>

#include <array>
#include <cmath>

#include "PumpController.h"

namespace cfg {
constexpr uint8_t kMotorCount = 4;
constexpr uint8_t kI2cAddress = 0x2A;
constexpr uint8_t kI2cSda = 8;
constexpr uint8_t kI2cScl = 9;
constexpr uint16_t kControlTickMs = 10;
constexpr int kMicroStepping = 8;
constexpr float kStepAngleDeg = 1.8f;
constexpr int kLedcResolutionBits = 8;

#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr std::array<uint8_t, kMotorCount> kPinStep = {4, 5, 6, 7};
constexpr std::array<uint8_t, kMotorCount> kPinDir = {10, 11, 12, 13};
constexpr std::array<uint8_t, kMotorCount> kPinEnable = {14, 15, 16, 17};
#else
constexpr std::array<uint8_t, kMotorCount> kPinStep = {25, 26, 27, 14};
constexpr std::array<uint8_t, kMotorCount> kPinDir = {33, 32, 15, 4};
constexpr std::array<uint8_t, kMotorCount> kPinEnable = {13, 12, 2, 5};
#endif
}  // namespace cfg

namespace exproto {
constexpr uint8_t kMagicA = 0x50;
constexpr uint8_t kMagicB = 0x58;
constexpr uint8_t kProtoVer = 1;
constexpr uint8_t kCmdHello = 0x01;
constexpr uint8_t kCmdGetState = 0x10;
constexpr uint8_t kCmdSetFlow = 0x20;
constexpr uint8_t kCmdStartDosing = 0x21;
constexpr uint8_t kCmdStop = 0x22;
constexpr uint8_t kCmdStart = 0x23;
constexpr uint8_t kCmdSetSettings = 0x24;
}  // namespace exproto

std::array<pump::PumpController, cfg::kMotorCount> controllers = {
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
    pump::PumpController(pump::Config{}),
};

uint32_t lastControlMs = 0;
const float kStepsPerRevolution = (360.0f / cfg::kStepAngleDeg) * cfg::kMicroStepping;

uint8_t txBuffer[40] = {0};
size_t txLen = 0;

uint8_t frameCrc(const uint8_t* data, size_t lenWithoutCrc) {
  uint8_t crc = 0;
  for (size_t i = 0; i < lenWithoutCrc; ++i) crc ^= data[i];
  return crc;
}

float speedToFrequency(float speed) {
  return fabsf(speed) * kStepsPerRevolution / 60.0f;
}

void setDriverFrequencyHz(uint8_t motor, float freqHz) {
  const uint8_t channel = motor;
  if (freqHz < 1.0f) {
    ledcWriteTone(channel, 0);
    digitalWrite(cfg::kPinStep[motor], LOW);
    digitalWrite(cfg::kPinEnable[motor], HIGH);
    return;
  }
  digitalWrite(cfg::kPinEnable[motor], LOW);
  ledcWriteTone(channel, freqHz);
}

void applyMotorSpeed(uint8_t motor, float speed) {
  const auto& conf = controllers[motor].config();
  if (fabsf(speed) < conf.minSpeed) {
    setDriverFrequencyHz(motor, 0);
    return;
  }
  digitalWrite(cfg::kPinDir[motor], speed >= 0 ? LOW : HIGH);
  setDriverFrequencyHz(motor, speedToFrequency(speed));
}

void encodeU16(uint8_t* out, int pos, uint16_t v) {
  out[pos] = static_cast<uint8_t>(v & 0xFF);
  out[pos + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void encodeU32(uint8_t* out, int pos, uint32_t v) {
  out[pos] = static_cast<uint8_t>(v & 0xFF);
  out[pos + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
  out[pos + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
  out[pos + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

uint16_t decodeU16(const uint8_t* in, int pos) {
  return static_cast<uint16_t>(in[pos]) | (static_cast<uint16_t>(in[pos + 1]) << 8);
}

void setHelloResponse() {
  txLen = 6;
  txBuffer[0] = exproto::kMagicA;
  txBuffer[1] = exproto::kMagicB;
  txBuffer[2] = exproto::kProtoVer;
  txBuffer[3] = cfg::kMotorCount;
  txBuffer[4] = 0;
  txBuffer[5] = frameCrc(txBuffer, 5);
}

void setStateResponse(uint8_t motor) {
  const auto& st = controllers[motor].state();
  txLen = 29;
  txBuffer[0] = (st.mode == pump::Mode::DOSING) ? 1 : 0;
  txBuffer[1] = st.running ? 1 : 0;
  encodeU16(txBuffer, 2, static_cast<uint16_t>(static_cast<int16_t>(roundf(st.targetSpeed * 10.0f))));
  encodeU16(txBuffer, 4, static_cast<uint16_t>(static_cast<int16_t>(roundf(st.currentSpeed * 10.0f))));
  encodeU16(txBuffer, 6, static_cast<uint16_t>(roundf(fmaxf(st.dosingRemainingMl, 0.0f))));
  encodeU32(txBuffer, 8, st.totalMotorUptimeSec);
  encodeU32(txBuffer, 12, static_cast<uint32_t>(round(st.totalPumpedVolumeL * 1000.0)));
  encodeU32(txBuffer, 16, static_cast<uint32_t>(round(st.totalHoseVolumeL * 1000.0)));
  encodeU16(txBuffer, 20, static_cast<uint16_t>(roundf(st.mlPerRevCw * 100.0f)));
  encodeU16(txBuffer, 22, static_cast<uint16_t>(roundf(st.mlPerRevCcw * 100.0f)));
  encodeU16(txBuffer, 24, static_cast<uint16_t>(roundf(st.dosingSpeed * 10.0f)));
  encodeU16(txBuffer, 26, static_cast<uint16_t>(roundf(controllers[motor].config().maxSpeed * 10.0f)));
  txBuffer[28] = frameCrc(txBuffer, 28);
}

void handleFrame(const uint8_t* frame, size_t len) {
  txLen = 0;
  if (len < 2) return;
  if (frame[len - 1] != frameCrc(frame, len - 1)) return;

  const uint8_t cmd = frame[0];
  if (cmd == exproto::kCmdHello) {
    setHelloResponse();
    return;
  }
  if (len < 3) return;

  const uint8_t motor = frame[1];
  if (motor >= cfg::kMotorCount) return;
  auto& ctrl = controllers[motor];

  if (cmd == exproto::kCmdGetState) {
    setStateResponse(motor);
    return;
  }
  if (cmd == exproto::kCmdSetFlow && len >= 6) {
    const uint16_t lphX10 = decodeU16(frame, 2);
    const bool reverse = frame[4] != 0;
    const auto& st = ctrl.state();
    const float lph = static_cast<float>(lphX10) / 10.0f;
    const float mlPerRev = reverse ? st.mlPerRevCcw : st.mlPerRevCw;
    const float speed = (lph * 1000.0f / 60.0f) / mlPerRev * (reverse ? -1.0f : 1.0f);
    ctrl.setSpeed(speed, pump::Mode::FLOW);
    return;
  }
  if (cmd == exproto::kCmdStartDosing && len >= 6) {
    const uint16_t volume = decodeU16(frame, 2);
    const bool reverse = frame[4] != 0;
    ctrl.startDosing(reverse ? -static_cast<int32_t>(volume) : static_cast<int32_t>(volume));
    return;
  }
  if (cmd == exproto::kCmdStop) {
    ctrl.stop(false);
    return;
  }
  if (cmd == exproto::kCmdStart) {
    ctrl.start();
    return;
  }
  if (cmd == exproto::kCmdSetSettings && len >= 11) {
    const float mlCw = static_cast<float>(decodeU16(frame, 2)) / 100.0f;
    const float mlCcw = static_cast<float>(decodeU16(frame, 4)) / 100.0f;
    const float dosingFlowLph = static_cast<float>(decodeU16(frame, 6)) / 10.0f;
    const float maxFlowLph = static_cast<float>(decodeU16(frame, 8)) / 10.0f;
    if (mlCw > 0.0f || mlCcw > 0.0f) ctrl.setMlPerRev(mlCw, mlCcw);
    if (dosingFlowLph > 0.0f) {
      const float refMlPerRev = ctrl.state().mlPerRevCw > 0.0f ? ctrl.state().mlPerRevCw : 2.6f;
      ctrl.setDosingSpeed((dosingFlowLph * 1000.0f / 60.0f) / refMlPerRev);
    }
    if (maxFlowLph > 0.0f) {
      const float refMlPerRev = ctrl.state().mlPerRevCw > 0.0f ? ctrl.state().mlPerRevCw : 2.6f;
      ctrl.setMaxSpeed((maxFlowLph * 1000.0f / 60.0f) / refMlPerRev);
    }
    return;
  }
}

void onI2cReceive(int len) {
  if (len <= 0 || len > 32) return;
  uint8_t buf[32] = {0};
  int i = 0;
  while (Wire.available() && i < len) {
    buf[i++] = Wire.read();
  }
  handleFrame(buf, static_cast<size_t>(i));
}

void onI2cRequest() {
  if (txLen > 0) {
    Wire.write(txBuffer, txLen);
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < cfg::kMotorCount; ++i) {
    pinMode(cfg::kPinStep[i], OUTPUT);
    pinMode(cfg::kPinDir[i], OUTPUT);
    pinMode(cfg::kPinEnable[i], OUTPUT);
    digitalWrite(cfg::kPinEnable[i], HIGH);
    ledcSetup(i, 1, cfg::kLedcResolutionBits);
    ledcAttachPin(cfg::kPinStep[i], i);
  }

  Wire.begin(cfg::kI2cAddress, cfg::kI2cSda, cfg::kI2cScl, 400000);
  Wire.onReceive(onI2cReceive);
  Wire.onRequest(onI2cRequest);
  lastControlMs = millis();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastControlMs < cfg::kControlTickMs) return;
  const uint32_t delta = now - lastControlMs;
  lastControlMs = now;
  for (uint8_t i = 0; i < cfg::kMotorCount; ++i) {
    controllers[i].tick(delta);
    applyMotorSpeed(i, controllers[i].state().currentSpeed);
  }
}
