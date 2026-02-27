#include <unity.h>

#include "PumpController.h"

namespace {

pump::Config makeConfig() {
  pump::Config cfg;
  cfg.maxSpeed = 450.0f;
  cfg.minSpeed = 0.01f;
  cfg.speedAccelPerSec = 50.0f;
  cfg.speedHaltPerSec = 200.0f;
  cfg.mlPerRevCw = 2.6f;
  cfg.mlPerRevCcw = 2.6f;
  return cfg;
}

void test_set_speed_clamps_and_runs() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(999.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 450.0f, ctrl.state().targetSpeed);
  TEST_ASSERT_TRUE(ctrl.state().running);

  ctrl.setSpeed(-999.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -450.0f, ctrl.state().targetSpeed);
}

void test_accelerates_by_rate() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(100.0f);
  ctrl.tick(10);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, ctrl.state().currentSpeed);

  ctrl.tick(10);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, ctrl.state().currentSpeed);
}

void test_halts_with_higher_rate() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(100.0f);
  for (int i = 0; i < 50; ++i) {
    ctrl.tick(10);
  }
  TEST_ASSERT_TRUE(ctrl.state().currentSpeed > 0.0f);

  ctrl.stop(false);
  ctrl.tick(10);
  // Halt step is 2 speed per tick (200 speed/s * 10ms).
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 23.0f, ctrl.state().currentSpeed);
}

void test_dosing_finishes_and_stops() {
  pump::PumpController ctrl(makeConfig());
  ctrl.startDosing(10);

  for (int i = 0; i < 2500; ++i) {
    ctrl.tick(10);
    if (!ctrl.state().running && ctrl.state().dosingRemainingMl <= 0.0f) {
      break;
    }
  }

  TEST_ASSERT_FALSE(ctrl.state().running);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, ctrl.state().dosingRemainingMl);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(pump::Mode::FLOW), static_cast<uint8_t>(ctrl.state().mode));
}

void test_volume_is_integrated() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(60.0f);

  // Reach target and run for 1 second total in 10ms ticks.
  for (int i = 0; i < 100; ++i) {
    ctrl.tick(10);
  }

  TEST_ASSERT_TRUE(ctrl.state().totalPumpedVolumeL > 0.0);
  TEST_ASSERT_TRUE(ctrl.state().totalHoseVolumeL > 0.0);
}

void test_uptime_accumulates_from_small_ticks() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(120.0f);

  for (int i = 0; i < 200; ++i) {
    ctrl.tick(10);
  }

  TEST_ASSERT_TRUE(ctrl.state().totalMotorUptimeSec >= 1);
}

void test_max_speed_change_reclamps_targets() {
  pump::PumpController ctrl(makeConfig());
  ctrl.setSpeed(300.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.0f, ctrl.state().targetSpeed);

  ctrl.setMaxSpeed(200.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, ctrl.config().maxSpeed);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, ctrl.state().targetSpeed);
}

}  // namespace

void run_tests() {
  UNITY_BEGIN();
  RUN_TEST(test_set_speed_clamps_and_runs);
  RUN_TEST(test_accelerates_by_rate);
  RUN_TEST(test_halts_with_higher_rate);
  RUN_TEST(test_dosing_finishes_and_stops);
  RUN_TEST(test_volume_is_integrated);
  RUN_TEST(test_uptime_accumulates_from_small_ticks);
  RUN_TEST(test_max_speed_change_reclamps_targets);
  UNITY_END();
}

#ifdef ARDUINO
void setup() { run_tests(); }
void loop() {}
#else
int main(int, char**) {
  run_tests();
  return 0;
}
#endif
