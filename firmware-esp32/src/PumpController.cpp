#include "PumpController.h"

#include <algorithm>
#include <cmath>

namespace pump {

PumpController::PumpController(Config cfg) : cfg_(cfg) {
  state_.mlPerRevCw = cfg_.mlPerRevCw;
  state_.mlPerRevCcw = cfg_.mlPerRevCcw;
}

const Config& PumpController::config() const { return cfg_; }

const State& PumpController::state() const { return state_; }

State& PumpController::mutableState() { return state_; }

void PumpController::setSpeed(float speed, Mode mode) {
  state_.mode = mode;
  state_.targetSpeed = std::max(-cfg_.maxSpeed, std::min(speed, cfg_.maxSpeed));

  if (std::fabs(state_.targetSpeed) < cfg_.minSpeed) {
    state_.targetSpeed = 0.0f;
    state_.running = false;
    return;
  }

  state_.running = true;
  state_.lastManualSpeed = state_.targetSpeed;
}

void PumpController::start() {
  float speed = state_.lastManualSpeed;
  if (std::fabs(speed) < cfg_.minSpeed) speed = 120.0f;
  setSpeed(speed, Mode::FLOW);
}

void PumpController::stop(bool emergency) {
  state_.targetSpeed = 0.0f;
  state_.mode = Mode::FLOW;
  state_.running = false;
  if (emergency) {
    state_.currentSpeed = 0.0f;
  }
}

void PumpController::startDosing(std::int32_t volumeMl) {
  if (volumeMl == 0) {
    stop(false);
    return;
  }

  state_.mode = Mode::DOSING;
  state_.dosingRemainingMl = std::fabs(static_cast<float>(volumeMl));
  setSpeed(volumeMl > 0 ? std::fabs(state_.dosingSpeed) : -std::fabs(state_.dosingSpeed), Mode::DOSING);
}

void PumpController::setMlPerRev(float cw, float ccw) {
  if (cw > 0.0f) state_.mlPerRevCw = cw;
  if (ccw > 0.0f) state_.mlPerRevCcw = ccw;
}

void PumpController::setDosingSpeed(float speed) {
  if (std::fabs(speed) < cfg_.minSpeed) return;
  state_.dosingSpeed = std::max(cfg_.minSpeed, std::min(std::fabs(speed), cfg_.maxSpeed));
}

void PumpController::setMaxSpeed(float speed) {
  if (speed < 1.0f) return;
  cfg_.maxSpeed = speed;
  state_.targetSpeed = std::max(-cfg_.maxSpeed, std::min(state_.targetSpeed, cfg_.maxSpeed));
  state_.currentSpeed = std::max(-cfg_.maxSpeed, std::min(state_.currentSpeed, cfg_.maxSpeed));
  state_.lastManualSpeed = std::max(-cfg_.maxSpeed, std::min(state_.lastManualSpeed, cfg_.maxSpeed));
  state_.dosingSpeed = std::max(cfg_.minSpeed, std::min(std::fabs(state_.dosingSpeed), cfg_.maxSpeed));
}

void PumpController::tick(std::uint32_t deltaMs) {
  if (deltaMs == 0) return;

  const float dtSec = static_cast<float>(deltaMs) / 1000.0f;
  const bool targetIsStop = std::fabs(state_.targetSpeed) < cfg_.minSpeed;
  const float ramp = (targetIsStop ? cfg_.speedHaltPerSec : cfg_.speedAccelPerSec) * dtSec;

  if (state_.currentSpeed < state_.targetSpeed) {
    state_.currentSpeed = std::min(state_.currentSpeed + ramp, state_.targetSpeed);
  } else if (state_.currentSpeed > state_.targetSpeed) {
    state_.currentSpeed = std::max(state_.currentSpeed - ramp, state_.targetSpeed);
  }

  if (std::fabs(state_.currentSpeed) < cfg_.minSpeed && targetIsStop) {
    state_.currentSpeed = 0.0f;
    state_.running = false;
  }

  const float mlPerRev = state_.currentSpeed >= 0 ? state_.mlPerRevCw : state_.mlPerRevCcw;
  const float deltaMl = std::fabs(state_.currentSpeed) / 60.0f * mlPerRev * dtSec;

  state_.totalPumpedVolumeL += deltaMl / 1000.0;
  state_.totalHoseVolumeL += deltaMl / 1000.0;

  if (std::fabs(state_.currentSpeed) >= cfg_.minSpeed) {
    uptimeRemainderMs_ += deltaMs;
    state_.totalMotorUptimeSec += (uptimeRemainderMs_ / 1000);
    uptimeRemainderMs_ %= 1000;
  }

  if (state_.mode == Mode::DOSING && state_.dosingRemainingMl > 0) {
    state_.dosingRemainingMl -= deltaMl;
    if (state_.dosingRemainingMl <= 0) {
      state_.dosingRemainingMl = 0;
      stop(false);
    }
  }
}

}  // namespace pump
