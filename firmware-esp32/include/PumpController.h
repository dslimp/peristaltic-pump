#pragma once

#include <cstdint>

namespace pump {

enum class Mode : uint8_t {
  FLOW = 0,
  DOSING = 1,
};

struct Config {
  float maxSpeed = 450.0f;
  float minSpeed = 0.01f;
  float speedAccelPerSec = 50.0f;
  float speedHaltPerSec = 200.0f;
  float mlPerRevCw = 2.6f;
  float mlPerRevCcw = 2.6f;
};

struct State {
  Mode mode = Mode::FLOW;
  float targetSpeed = 0.0f;
  float currentSpeed = 0.0f;
  float lastManualSpeed = 120.0f;
  float mlPerRevCw = 2.6f;
  float mlPerRevCcw = 2.6f;
  float dosingSpeed = 180.0f;
  bool running = false;
  float dosingRemainingMl = 0.0f;
  std::uint32_t totalMotorUptimeSec = 0;
  double totalPumpedVolumeL = 0.0;
  double totalHoseVolumeL = 0.0;
};

class PumpController {
 public:
  explicit PumpController(Config cfg);

  const Config& config() const;
  const State& state() const;
  State& mutableState();

  void setSpeed(float speed, Mode mode = Mode::FLOW);
  void start();
  void stop(bool emergency = false);
  void startDosing(std::int32_t volumeMl);
  void setMlPerRev(float cw, float ccw);
  void setDosingSpeed(float speed);
  void setMaxSpeed(float speed);

  // Simulates elapsed time and updates speed, uptime and volume counters.
  void tick(std::uint32_t deltaMs);

 private:
  Config cfg_;
  State state_;
  std::uint32_t uptimeRemainderMs_ = 0;
};

}  // namespace pump
