(function initGrowthScheduleApi(root) {
  const DAY_MINUTES = 24 * 60;

  function clampHour(value, fallback = 9) {
    const v = Number(value);
    if (!Number.isFinite(v)) return fallback;
    return Math.max(0, Math.min(23, Math.round(v)));
  }

  function clampMinute(value, fallback = 0) {
    const v = Number(value);
    if (!Number.isFinite(v)) return fallback;
    return Math.max(0, Math.min(59, Math.round(v)));
  }

  function clampPauseMinutes(value, fallback = 10) {
    const v = Number(value);
    if (!Number.isFinite(v)) return fallback;
    return Math.max(1, Math.min(180, Math.round(v)));
  }

  function normalizeMinutes(value) {
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) return 0;
    const rounded = Math.round(numeric);
    return ((rounded % DAY_MINUTES) + DAY_MINUTES) % DAY_MINUTES;
  }

  function minutesFromTime(hour, minute) {
    return (clampHour(hour, 0) * 60) + clampMinute(minute, 0);
  }

  function timeFromMinutes(totalMinutes) {
    const normalized = normalizeMinutes(totalMinutes);
    return {
      hour: Math.floor(normalized / 60),
      minute: normalized % 60,
      totalMinutes: normalized,
    };
  }

  function shiftTimeByMinutes(hour, minute, deltaMinutes) {
    const baseMinutes = minutesFromTime(hour, minute);
    const delta = Math.max(0, Math.round(Number(deltaMinutes) || 0));
    return timeFromMinutes(baseMinutes + delta);
  }

  function truncateUtf8(value, maxBytes) {
    const src = String(value || '');
    const limit = Math.max(1, Math.round(Number(maxBytes) || 1));
    if (typeof TextEncoder === 'undefined') return src.slice(0, limit);
    const encoder = new TextEncoder();
    if (encoder.encode(src).length <= limit) return src;
    let out = '';
    let bytes = 0;
    for (const ch of src) {
      const partBytes = encoder.encode(ch).length;
      if (bytes + partBytes > limit) break;
      out += ch;
      bytes += partBytes;
    }
    return out;
  }

  function generateGrowthScheduleEntries(options = {}) {
    const programName = String(options.programName || '').trim();
    const phase = options.phase || {};
    const nutrients = phase.nutrients || {};
    const ph = phase.ph || {};
    const waterL = Math.max(1, Number(options.waterL) || 1);
    const phRegulationEnabled = !!options.phRegulationEnabled;
    const nutrientMask = Number(options.nutrientMask) || 0;
    const phMask = Number(options.phMask) || 0;
    const pauseMinutes = clampPauseMinutes(options.pauseMinutes, 10);
    const maxMotor = Math.max(0, (Number(options.activeMotorCount) || 1) - 1);
    const nameResolver = (typeof options.nameResolver === 'function')
      ? options.nameResolver
      : ((key) => String(key || ''));
    const phScale = waterL / 10.0;

    const channelPlan = [];
    const pushChannel = (channel) => {
      if (!Number.isFinite(channel.volumeMl) || channel.volumeMl <= 0) return;
      channelPlan.push({
        ...channel,
        baseMinutes: normalizeMinutes(channel.baseMinutes),
        order: channelPlan.length,
      });
    };

    if (phRegulationEnabled) {
      pushChannel({
        nameKey: 'growth_pump_ph_plus',
        motorId: 0,
        volumeMl: Number(ph.plus) * phScale,
        baseMinutes: shiftTimeByMinutes(options.phHour, options.phMinute, 0).totalMinutes,
        weekdaysMask: phMask,
      });
      pushChannel({
        nameKey: 'growth_pump_ph_minus',
        motorId: 1,
        volumeMl: Number(ph.minus) * phScale,
        baseMinutes: shiftTimeByMinutes(options.phHour, options.phMinute, pauseMinutes).totalMinutes,
        weekdaysMask: phMask,
      });
    }

    pushChannel({
      nameKey: 'growth_pump_nutrient_a',
      motorId: 2,
      volumeMl: Number(nutrients.a) * waterL,
      baseMinutes: shiftTimeByMinutes(options.nutrientHour, options.nutrientMinute, 0).totalMinutes,
      weekdaysMask: nutrientMask,
    });
    pushChannel({
      nameKey: 'growth_pump_nutrient_b',
      motorId: 3,
      volumeMl: Number(nutrients.b) * waterL,
      baseMinutes: shiftTimeByMinutes(options.nutrientHour, options.nutrientMinute, pauseMinutes).totalMinutes,
      weekdaysMask: nutrientMask,
    });
    pushChannel({
      nameKey: 'growth_pump_nutrient_c',
      motorId: 4,
      volumeMl: Number(nutrients.c) * waterL,
      baseMinutes: shiftTimeByMinutes(options.nutrientHour, options.nutrientMinute, pauseMinutes * 2).totalMinutes,
      weekdaysMask: nutrientMask,
    });

    const ordered = [...channelPlan].sort((a, b) => {
      if (a.baseMinutes !== b.baseMinutes) return a.baseMinutes - b.baseMinutes;
      return a.order - b.order;
    });

    let lastScheduled = null;
    for (const item of ordered) {
      let planned = item.baseMinutes;
      if (lastScheduled !== null) planned = Math.max(planned, lastScheduled + pauseMinutes);
      item.scheduledMinutes = planned;
      lastScheduled = planned;
    }

    let remapped = 0;
    const entries = ordered.map((item) => {
      const mappedMotor = Math.max(0, Math.min(Number(item.motorId) || 0, maxMotor));
      if (mappedMotor !== item.motorId) remapped += 1;
      const at = timeFromMinutes(item.scheduledMinutes);
      return {
        name: truncateUtf8(`${programName} ${nameResolver(item.nameKey)}`, 32),
        motorId: mappedMotor,
        enabled: true,
        hour: at.hour,
        minute: at.minute,
        volumeMl: Math.max(1, Math.round(item.volumeMl)),
        reverse: false,
        weekdaysMask: item.weekdaysMask,
      };
    });

    return { entries, remapped, pauseMinutes };
  }

  const api = {
    clampHour,
    clampMinute,
    clampPauseMinutes,
    shiftTimeByMinutes,
    truncateUtf8,
    generateGrowthScheduleEntries,
  };

  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  root.GrowthSchedule = api;
}(typeof globalThis !== 'undefined' ? globalThis : this));
