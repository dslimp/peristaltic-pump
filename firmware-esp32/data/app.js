async function req(path, method = 'GET', body = null) {
  const r = await fetch(path, {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined,
  });
  return r.json();
}

async function post(path, body) {
  await req(path, 'POST', body);
  await refresh();
}

const I18N = {
  en: {
    titleMain: 'Peristaltic Pump',
    subtitleMain: 'Local control panel',
    chipModeLabel: 'mode',
    chipDirLabel: 'dir',
    chipTimeLabel: 'time',
    tabControlBtn: 'Control',
    tabCalBtn: 'Calibration / Settings',
    tabScheduleBtn: 'Schedule',
    tabGrowthBtn: 'Growth Program',
    tabHaBtn: 'Home Assistant',
    driveTitle: 'Drive',
    motorLabel: 'Motor',
    scheduleMotorLabel: 'Motor',
    expansionEnabledLabel: 'Expansion enabled',
    expansionInterfaceLabel: 'Expansion interface',
    expansionMotorCountLabel: 'Expansion motors (0-4)',
    stopBtn: 'Stop',
    reverseLabel: 'Reverse',
    flowLabel: 'Flow (L/h)',
    startFlowBtn: 'Start',
    dosingLabel: 'Dosing (ml)',
    startDosingBtn: 'Start',
    liveMetricsTitle: 'Live Metrics',
    metricModeLabel: 'Mode',
    metricDirectionLabel: 'Direction',
    metricCurrentFlowLabel: 'Current Flow (L/h)',
    metricTotalPumpedLabel: 'Total Pumped (L)',
    metricDosingLeftLabel: 'Dosing Left (ml)',
    rawStateSummary: 'Raw State JSON',
    calSettingsTitle: 'Calibration / Settings',
    mlCwLabel: 'ml/rev CW',
    mlCcwLabel: 'ml/rev CCW',
    maxFlowLabel: 'Max Flow (L/h)',
    dosingFlowLabel: 'Dosing Flow (L/h)',
    motorAliasesTitle: 'Motor Aliases',
    ntpServerLabel: 'NTP Server',
    uiLanguageLabel: 'Language',
    growthProgramEnabledLabel: 'Enable Growth Program tab',
    phRegulationEnabledLabel: 'Enable pH regulation in growth schedule',
    saveSettingsBtn: 'Save Settings',
    saveUiLangBtn: 'Save Language',
    guidedCalTitle: 'Guided Calibration',
    dirLabel: 'Direction',
    revolutionsLabel: 'Revolutions',
    runCalibrationBtn: 'Run Calibration',
    measuredMlLabel: 'Measured ml',
    applyCalibrationBtn: 'Apply',
    webSecurityTitle: 'Web Security (Basic Auth)',
    authEnabledLabel: 'Enable authentication',
    usernameLabel: 'Username',
    passwordLabel: 'Password',
    saveAuthBtn: 'Save Auth',
    fwUpdateTitle: 'Firmware Update (GitHub Releases)',
    fwRepoLabel: 'GitHub Repo (owner/repo)',
    fwAssetLabel: 'Asset Name (.bin)',
    fwFsAssetLabel: 'Filesystem Asset (.bin)',
    fwLocalUrlLabel: 'Firmware URL (.bin)',
    fwLocalFsUrlLabel: 'Filesystem URL (.bin)',
    fwSaveConfigBtn: 'Save OTA Config',
    fwLoadReleasesBtn: 'Load Releases',
    fwUpdateLatestBtn: 'Update to Latest',
    fwUpdateUrlBtn: 'Update from URL',
    fwReleaseSelectLabel: 'Release Tag',
    fwUpdateSelectedBtn: 'Update Selected',
    fwStatusText: 'Firmware update is idle.',
    fw_release_loading: 'Loading releases...',
    fw_release_loaded: 'Loaded releases',
    fw_release_missing: 'Set GitHub repo first',
    fw_update_started: 'Update started. Device may reboot in ~10-30 seconds.',
    fw_update_url_started: 'Downloading OTA images from provided URLs...',
    fw_update_failed: 'Update failed',
    fw_update_no_releases: 'No releases available',
    fw_update_url_missing: 'Set firmware and filesystem URLs first',
    scheduleTitle: 'Schedule Dosing',
    growthTitle: 'Growth Program',
    growthPlantLabel: 'Plant',
    growthFertilizerLabel: 'Fertilizer Type',
    growthProfileLabel: 'Program',
    growthPhaseLabel: 'Plant Phase',
    growthWaterVolumeLabel: 'Water Volume (L)',
    growthRecalculateBtn: 'Recalculate',
    growthCreateScheduleBtn: 'Create Schedule',
    growthExportBtn: 'Export JSON',
    growthImportBtn: 'Import JSON',
    growthScheduleTitle: 'Recommended Frequency',
    growthNutrientHourLabel: 'Nutrient time (hour)',
    growthNutrientMinuteLabel: 'Nutrient time (minute)',
    growthPhHourLabel: 'pH time (hour)',
    growthPhMinuteLabel: 'pH time (minute)',
    growthPauseMinutesLabel: 'Pause between channels (min)',
    growth_pump_ph_plus: 'Pump 1: pH+',
    growth_pump_ph_minus: 'Pump 2: pH-',
    growth_pump_nutrient_a: 'Pump 3: Nutrient A',
    growth_pump_nutrient_b: 'Pump 4: Nutrient B',
    growth_pump_nutrient_c: 'Pump 5: Nutrient C',
    growth_profile_universal: 'Universal',
    growth_profile_strawberry: 'Strawberry',
    growth_profile_flowers: 'Flowers',
    growth_profile_leafy: 'Leafy Greens',
    growth_profile_tomato: 'Tomatoes',
    growth_phase_seedling: 'Seedling',
    growth_phase_vegetative: 'Vegetative',
    growth_phase_flowering: 'Flowering',
    growth_phase_fruiting: 'Fruiting',
    growth_feedings_week: 'feedings/week',
    growth_ph_adjustments_week: 'pH corrections/week',
    growth_weekly_total: 'Weekly total',
    growth_per_dose: 'Per dose',
    growth_schedule_created: 'Growth schedule saved',
    growth_schedule_ph_off: 'pH regulation is off',
    growth_schedule_motors_warning: 'some channels were mapped to available motors',
    growth_import_status_idle: 'Import/export growth programs (JSON v1).',
    growth_import_ok: 'Growth programs imported',
    growth_export_ok: 'Growth programs exported',
    growth_import_err: 'Import failed',
    tzOffsetLabel: 'Timezone Offset (minutes)',
    schHourLabel: 'Hour',
    schMinuteLabel: 'Minute',
    schNameLabel: 'Name',
    schVolumeLabel: 'Volume (ml)',
    schReverseLabel: 'Reverse',
    schEnabledLabel: 'Enabled',
    daysOfWeekLabel: 'Days of week',
    addScheduleBtn: 'Add Entry',
    cancelScheduleEditBtn: 'Cancel Edit',
    schedule_edit: 'Edit',
    schedule_update: 'Update Entry',
    schedule_rule: 'Rule',
    saveScheduleBtn: 'Save Schedule',
    resetWifiBtn: 'Reset Wi-Fi (AP setup)',
    mode_idle: 'idle',
    mode_flow_lph: 'flow_lph',
    mode_dosing: 'dosing',
    dir_forward: 'forward',
    dir_reverse: 'reverse',
    dir_none: '-',
    sched_enabled: 'enabled',
    sched_disabled: 'disabled',
    sched_reverse: 'reverse',
    sched_forward: 'forward',
    sched_daily: 'daily',
    schedule_empty: 'No schedule entries.',
    schedule_delete: 'Delete',
    motor_card: 'Motor',
    days: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
  },
  ru: {
    titleMain: 'Перистальтический насос',
    subtitleMain: 'Локальная панель управления',
    chipModeLabel: 'режим',
    chipDirLabel: 'напр',
    chipTimeLabel: 'время',
    tabControlBtn: 'Управление',
    tabCalBtn: 'Калибровка / Настройки',
    tabScheduleBtn: 'Расписание',
    tabGrowthBtn: 'Программа роста',
    tabHaBtn: 'Home Assistant',
    driveTitle: 'Привод',
    motorLabel: 'Мотор',
    scheduleMotorLabel: 'Мотор',
    expansionEnabledLabel: 'Модуль расширения включен',
    expansionInterfaceLabel: 'Expansion interface',
    expansionMotorCountLabel: 'Моторов расширения (0-4)',
    stopBtn: 'Стоп',
    reverseLabel: 'Реверс',
    flowLabel: 'Поток (L/h)',
    startFlowBtn: 'Старт',
    dosingLabel: 'Дозировка (ml)',
    startDosingBtn: 'Старт',
    liveMetricsTitle: 'Текущие метрики',
    metricModeLabel: 'Режим',
    metricDirectionLabel: 'Направление',
    metricCurrentFlowLabel: 'Текущий поток (L/h)',
    metricTotalPumpedLabel: 'Прокачано всего (L)',
    metricDosingLeftLabel: 'Остаток дозы (ml)',
    rawStateSummary: 'JSON',
    calSettingsTitle: 'Калибровка / Настройки',
    mlCwLabel: 'ml/оборот CW',
    mlCcwLabel: 'ml/оборот CCW',
    maxFlowLabel: 'Макс. поток (L/h)',
    dosingFlowLabel: 'Дозировка (L/h)',
    motorAliasesTitle: 'Алиасы моторов',
    ntpServerLabel: 'NTP Server',
    uiLanguageLabel: 'Язык',
    growthProgramEnabledLabel: 'Включить вкладку программы роста',
    phRegulationEnabledLabel: 'Включить pH-регуляцию в расписании роста',
    saveSettingsBtn: 'Сохранить настройки',
    saveUiLangBtn: 'Сохранить язык',
    guidedCalTitle: 'Пошаговая калибровка',
    dirLabel: 'Направление',
    revolutionsLabel: 'Обороты',
    runCalibrationBtn: 'Запустить калибровку',
    measuredMlLabel: 'Измерено ml',
    applyCalibrationBtn: 'Применить',
    webSecurityTitle: 'Web Security (Basic Auth)',
    authEnabledLabel: 'Включить авторизацию',
    usernameLabel: 'Логин',
    passwordLabel: 'Пароль',
    saveAuthBtn: 'Save Auth',
    fwUpdateTitle: 'Обновление прошивки (GitHub Releases)',
    fwRepoLabel: 'GitHub репозиторий (owner/repo)',
    fwAssetLabel: 'Имя ассета (.bin)',
    fwFsAssetLabel: 'Ассет файловой системы (.bin)',
    fwLocalUrlLabel: 'URL прошивки (.bin)',
    fwLocalFsUrlLabel: 'URL файловой системы (.bin)',
    fwSaveConfigBtn: 'Сохранить OTA конфиг',
    fwLoadReleasesBtn: 'Загрузить релизы',
    fwUpdateLatestBtn: 'Обновить до последнего',
    fwUpdateUrlBtn: 'Обновить по URL',
    fwReleaseSelectLabel: 'Тег релиза',
    fwUpdateSelectedBtn: 'Обновить выбранный',
    fwStatusText: 'Ожидание обновления прошивки.',
    fw_release_loading: 'Загрузка релизов...',
    fw_release_loaded: 'Релизы загружены',
    fw_release_missing: 'Сначала укажите GitHub repo',
    fw_update_started: 'Обновление запущено. Устройство может перезагрузиться через ~10-30 секунд.',
    fw_update_url_started: 'Загрузка OTA-образов по указанным URL...',
    fw_update_failed: 'Ошибка обновления',
    fw_update_no_releases: 'Нет доступных релизов',
    fw_update_url_missing: 'Сначала укажите URL прошивки и файловой системы',
    scheduleTitle: 'Расписание дозировки',
    growthTitle: 'Программа роста',
    growthPlantLabel: 'Растение',
    growthFertilizerLabel: 'Тип удобрения',
    growthProfileLabel: 'Программа',
    growthPhaseLabel: 'Фаза растения',
    growthWaterVolumeLabel: 'Объем воды (L)',
    growthRecalculateBtn: 'Пересчитать',
    growthCreateScheduleBtn: 'Создать расписание',
    growthExportBtn: 'Экспорт JSON',
    growthImportBtn: 'Импорт JSON',
    growthScheduleTitle: 'Рекомендуемая частота',
    growthNutrientHourLabel: 'Время удобрений (час)',
    growthNutrientMinuteLabel: 'Время удобрений (минута)',
    growthPhHourLabel: 'Время pH (час)',
    growthPhMinuteLabel: 'Время pH (минута)',
    growthPauseMinutesLabel: 'Пауза между каналами (мин)',
    growth_pump_ph_plus: 'Насос 1: pH+',
    growth_pump_ph_minus: 'Насос 2: pH-',
    growth_pump_nutrient_a: 'Насос 3: Удобрение A',
    growth_pump_nutrient_b: 'Насос 4: Удобрение B',
    growth_pump_nutrient_c: 'Насос 5: Удобрение C',
    growth_profile_universal: 'Общий',
    growth_profile_strawberry: 'Земляника',
    growth_profile_flowers: 'Цветы',
    growth_profile_leafy: 'Листовая зелень',
    growth_profile_tomato: 'Томаты',
    growth_phase_seedling: 'Рассада',
    growth_phase_vegetative: 'Вегетация',
    growth_phase_flowering: 'Цветение',
    growth_phase_fruiting: 'Плодоношение',
    growth_feedings_week: 'внесений в неделю',
    growth_ph_adjustments_week: 'коррекций pH в неделю',
    growth_weekly_total: 'Всего в неделю',
    growth_per_dose: 'За одно внесение',
    growth_schedule_created: 'Расписание роста сохранено',
    growth_schedule_ph_off: 'pH-регуляция выключена',
    growth_schedule_motors_warning: 'часть каналов была переназначена на доступные моторы',
    growth_import_status_idle: 'Импорт/экспорт программ роста (JSON v1).',
    growth_import_ok: 'Программы роста импортированы',
    growth_export_ok: 'Программы роста экспортированы',
    growth_import_err: 'Ошибка импорта',
    tzOffsetLabel: 'Timezone Offset (minutes)',
    schHourLabel: 'Час',
    schMinuteLabel: 'Минута',
    schNameLabel: 'Имя',
    schVolumeLabel: 'Объем (ml)',
    schReverseLabel: 'Реверс',
    schEnabledLabel: 'Включено',
    daysOfWeekLabel: 'Дни недели',
    addScheduleBtn: 'Добавить запись',
    cancelScheduleEditBtn: 'Отменить редактирование',
    schedule_edit: 'Редактировать',
    schedule_update: 'Сохранить запись',
    schedule_rule: 'Правило',
    saveScheduleBtn: 'Сохранить расписание',
    resetWifiBtn: 'Reset Wi-Fi (AP setup)',
    mode_idle: 'idle',
    mode_flow_lph: 'flow_lph',
    mode_dosing: 'dosing',
    dir_forward: 'вперед',
    dir_reverse: 'назад',
    dir_none: '-',
    sched_enabled: 'вкл',
    sched_disabled: 'выкл',
    sched_reverse: 'реверс',
    sched_forward: 'вперед',
    sched_daily: 'daily',
    schedule_empty: 'Нет записей расписания.',
    schedule_delete: 'Удалить',
    motor_card: 'Мотор',
    days: ['Пн', 'Вт', 'Ср', 'Чт', 'Пт', 'Сб', 'Вс'],
  },
};

let currentLanguage = 'en';
let currentMotorId = 0;
let activeMotorCount = 1;
let currentTzOffsetMinutes = 0;
let scheduleEntries = [];
let motorAliases = ['Motor 0'];
let editingScheduleIndex = -1;
let lastState = null;
let growthProgramEnabled = false;
let phRegulationEnabled = false;
let firmwareReleases = [];
const growthScheduler = globalThis.GrowthSchedule || null;
const GROWTH_SCHEMA = 'peristaltic.growth-programs';
const GROWTH_SCHEMA_VERSION = 1;
const GROWTH_STORAGE_KEY = 'peristaltic_growth_programs_v2';

const GROWTH_PHASE_KEYS = ['seedling', 'vegetative', 'flowering', 'fruiting'];

const DEFAULT_GROWTH_FERTILIZERS = [
  {
    id: 'aquatica-tripart',
    nameEn: 'Aquatica TriPart',
    nameRu: 'Aquatica TriPart',
    source: 'Terra Aquatica TriPart feed chart',
    phases: {
      seedling: { a: 0.6, b: 0.6, c: 0.6 },
      vegetative: { a: 1.5, b: 1.0, c: 0.5 },
      flowering: { a: 1.5, b: 1.5, c: 1.0 },
      fruiting: { a: 0.7, b: 1.4, c: 2.1 },
    },
  },
  {
    id: 'gh-floraseries',
    nameEn: 'General Hydroponics FloraSeries',
    nameRu: 'General Hydroponics FloraSeries',
    source: 'General Hydroponics FloraSeries usage guide',
    phases: {
      seedling: { a: 1.0, b: 1.0, c: 1.0 },
      vegetative: { a: 2.0, b: 1.0, c: 1.0 },
      flowering: { a: 1.0, b: 2.0, c: 3.0 },
      fruiting: { a: 1.0, b: 2.0, c: 3.0 },
    },
  },
  {
    id: 'an-ph-perfect',
    nameEn: 'Advanced Nutrients pH Perfect G/M/B',
    nameRu: 'Advanced Nutrients pH Perfect G/M/B',
    source: 'Advanced Nutrients pH Perfect feeding chart',
    phases: {
      seedling: { a: 1.0, b: 1.0, c: 1.0 },
      vegetative: { a: 2.0, b: 2.0, c: 2.0 },
      flowering: { a: 3.0, b: 3.0, c: 3.0 },
      fruiting: { a: 4.0, b: 4.0, c: 4.0 },
    },
  },
  {
    id: 'foxfarm-trio',
    nameEn: 'FoxFarm Hydro Trio',
    nameRu: 'FoxFarm Hydro Trio',
    source: 'FoxFarm Big Bloom / Grow Big / Tiger Bloom guides',
    phases: {
      seedling: { a: 0.5, b: 1.5, c: 0.2 },
      vegetative: { a: 2.5, b: 2.5, c: 0.5 },
      flowering: { a: 1.5, b: 3.0, c: 2.5 },
      fruiting: { a: 1.0, b: 3.0, c: 3.0 },
    },
  },
];

const DEFAULT_GROWTH_PLANTS = [
  { id: 'universal', nameEn: 'Universal', nameRu: 'Универсально', nutrientMul: 1.0, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.28, minus: 0.34 } },
  { id: 'lettuce', nameEn: 'Lettuce', nameRu: 'Салат', nutrientMul: 0.78, feedings: [2, 4, 4, 4], ph: [2, 3, 3, 3], phDose: { plus: 0.2, minus: 0.28 } },
  { id: 'basil', nameEn: 'Basil', nameRu: 'Базилик', nutrientMul: 0.9, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.24, minus: 0.3 } },
  { id: 'spinach', nameEn: 'Spinach', nameRu: 'Шпинат', nutrientMul: 0.82, feedings: [2, 4, 4, 4], ph: [2, 3, 3, 3], phDose: { plus: 0.22, minus: 0.28 } },
  { id: 'kale', nameEn: 'Kale', nameRu: 'Капуста кейл', nutrientMul: 0.9, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.25, minus: 0.31 } },
  { id: 'arugula', nameEn: 'Arugula', nameRu: 'Руккола', nutrientMul: 0.74, feedings: [2, 4, 4, 4], ph: [2, 3, 3, 3], phDose: { plus: 0.2, minus: 0.27 } },
  { id: 'mint', nameEn: 'Mint', nameRu: 'Мята', nutrientMul: 0.86, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.23, minus: 0.3 } },
  { id: 'parsley', nameEn: 'Parsley', nameRu: 'Петрушка', nutrientMul: 0.84, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.23, minus: 0.3 } },
  { id: 'cilantro', nameEn: 'Cilantro', nameRu: 'Кинза', nutrientMul: 0.8, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.22, minus: 0.29 } },
  { id: 'microgreens', nameEn: 'Microgreens', nameRu: 'Микрозелень', nutrientMul: 0.6, feedings: [2, 3, 3, 3], ph: [1, 2, 2, 2], phDose: { plus: 0.18, minus: 0.24 } },
  { id: 'strawberry', nameEn: 'Strawberry', nameRu: 'Клубника', nutrientMul: 0.92, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.26, minus: 0.33 } },
  { id: 'tomato', nameEn: 'Tomato', nameRu: 'Томат', nutrientMul: 1.08, feedings: [2, 3, 4, 5], ph: [2, 2, 3, 3], phDose: { plus: 0.32, minus: 0.38 } },
  { id: 'cucumber', nameEn: 'Cucumber', nameRu: 'Огурец', nutrientMul: 1.0, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.3, minus: 0.36 } },
  { id: 'pepper', nameEn: 'Pepper', nameRu: 'Перец', nutrientMul: 1.05, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.3, minus: 0.37 } },
  { id: 'flowers', nameEn: 'Flowers', nameRu: 'Цветы', nutrientMul: 0.95, feedings: [2, 3, 4, 4], ph: [2, 2, 3, 3], phDose: { plus: 0.26, minus: 0.32 } },
];

function buildGrowthProgram(fertilizer, plant) {
  const fertilizerNameEn = String(fertilizer?.nameEn || 'Fertilizer');
  const fertilizerNameRu = String(fertilizer?.nameRu || fertilizerNameEn);
  const plantNameEn = String(plant?.nameEn || 'Plant');
  const plantNameRu = String(plant?.nameRu || plantNameEn);
  const feedings = Array.isArray(plant?.feedings) ? plant.feedings : [2, 3, 4, 4];
  const phAdjustments = Array.isArray(plant?.ph) ? plant.ph : [2, 2, 3, 3];
  const nutrientMul = Math.max(0.1, Number(plant?.nutrientMul || 1.0));
  const phDose = plant?.phDose || { plus: 0.25, minus: 0.3 };
  const fertilizerPhases = fertilizer?.phases || {};
  const fertilizerId = String(fertilizer?.id || 'fertilizer');
  const plantId = String(plant?.id || 'plant');
  return {
    id: `${fertilizerId}-${plantId}`.toLowerCase().replace(/[^a-z0-9-_]/g, '-'),
    fertilizerId,
    fertilizerNameEn,
    fertilizerNameRu,
    plantId,
    plantNameEn,
    plantNameRu,
    nameEn: `${fertilizerNameEn} / ${plantNameEn}`,
    nameRu: `${fertilizerNameRu} / ${plantNameRu}`,
    source: String(fertilizer?.source || 'imported'),
    phases: GROWTH_PHASE_KEYS.map((key, idx) => {
      const base = fertilizerPhases[key] || { a: 1, b: 1, c: 1 };
      return {
        key,
        feedingsPerWeek: Math.max(1, Math.min(7, Number(feedings[idx] || 1))),
        phAdjustmentsPerWeek: Math.max(1, Math.min(7, Number(phAdjustments[idx] || 1))),
        nutrients: {
          a: Number((Math.max(0, Number(base.a || 0)) * nutrientMul).toFixed(2)),
          b: Number((Math.max(0, Number(base.b || 0)) * nutrientMul).toFixed(2)),
          c: Number((Math.max(0, Number(base.c || 0)) * nutrientMul).toFixed(2)),
        },
        ph: {
          plus: Math.max(0, Number(phDose.plus || 0)),
          minus: Math.max(0, Number(phDose.minus || 0)),
        },
      };
    }),
  };
}

function buildDefaultGrowthPrograms() {
  return DEFAULT_GROWTH_FERTILIZERS.flatMap((fertilizer) =>
    DEFAULT_GROWTH_PLANTS.map((plant) => buildGrowthProgram(fertilizer, plant))
  );
}

const DEFAULT_GROWTH_PROGRAMS = buildDefaultGrowthPrograms();
let growthPrograms = JSON.parse(JSON.stringify(DEFAULT_GROWTH_PROGRAMS));

function t(key) {
  return (I18N[currentLanguage] && I18N[currentLanguage][key]) || (I18N.en[key] || key);
}

function escapeHtml(value) {
  return String(value ?? '')
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;');
}

function motorLabel(motorId) {
  const id = Number(motorId) || 0;
  const alias = (motorAliases[id] || '').trim();
  return alias || `${t('motor_card')} ${id}`;
}

function syncAliasesFromMotors(motors) {
  if (!Array.isArray(motors) || motors.length === 0) return;
  const next = [...motorAliases];
  motors.forEach((m) => {
    const id = Number(m.motorId);
    if (!Number.isFinite(id) || id < 0) return;
    const alias = String(m.alias || '').trim();
    if (alias) next[id] = alias;
  });
  motorAliases = next;
}

function formatUtcOffset(minutes) {
  const sign = minutes >= 0 ? '+' : '-';
  const abs = Math.abs(minutes);
  const h = String(Math.floor(abs / 60)).padStart(2, '0');
  const m = String(abs % 60).padStart(2, '0');
  return `UTC${sign}${h}:${m}`;
}

function populateTimezoneSelect(selectedMinutes = 0) {
  const select = document.getElementById('tzOffsetMinutes');
  if (!select) return;
  const selected = Math.max(-720, Math.min(840, Number(selectedMinutes) || 0));
  if (select.options.length === 0) {
    for (let minutes = -720; minutes <= 840; minutes += 15) {
      const option = document.createElement('option');
      option.value = String(minutes);
      option.textContent = formatUtcOffset(minutes);
      select.appendChild(option);
    }
  }
  currentTzOffsetMinutes = selected;
  select.value = String(selected);
}

function normalizeAliases(count) {
  const limit = Math.max(1, Number(count) || 1);
  const next = [];
  for (let i = 0; i < limit; i += 1) {
    const alias = (motorAliases[i] || '').trim();
    next.push(alias || `${t('motor_card')} ${i}`);
  }
  motorAliases = next;
}

function renderMotorAliasesEditor() {
  const root = document.getElementById('motorAliasesEditor');
  if (!root) return;
  normalizeAliases(activeMotorCount);
  const liveValues = [];
  for (let i = 0; i < activeMotorCount; i += 1) {
    const existing = document.getElementById(`motorAlias${i}`);
    if (existing) liveValues[i] = String(existing.value || '');
  }
  root.innerHTML = motorAliases.map((alias, i) => `
    <div class="schedule-item">
      <div class="field" style="min-width:100%">
        <label>${t('motor_card')} ${i}</label>
        <input id="motorAlias${i}" type="text" maxlength="24" value="${escapeHtml(liveValues[i] ?? alias)}">
      </div>
    </div>
  `).join('');
}

function ensureMotorSelectors(count) {
  activeMotorCount = Math.max(1, Number(count) || 1);
  if (currentMotorId >= activeMotorCount) currentMotorId = 0;
  normalizeAliases(activeMotorCount);

  const render = (id, selected) => {
    const el = document.getElementById(id);
    if (!el) return;
    const prev = selected;
    el.innerHTML = '';
    for (let i = 0; i < activeMotorCount; i += 1) {
      const option = document.createElement('option');
      option.value = String(i);
      option.textContent = motorLabel(i);
      el.appendChild(option);
    }
    const next = Number.isFinite(prev) ? Math.min(Math.max(prev, 0), activeMotorCount - 1) : currentMotorId;
    el.value = String(next);
  };

  render('motorSelect', Number(document.getElementById('motorSelect')?.value ?? currentMotorId));
  render('schMotorId', Number(document.getElementById('schMotorId')?.value ?? currentMotorId));
  renderMotorAliasesEditor();
}

function setFirmwareStatus(text) {
  const el = document.getElementById('fwStatusText');
  if (el) el.textContent = String(text || '');
}

function selectedFirmwareRepo() {
  return String(document.getElementById('fwRepo')?.value || '').trim();
}

function selectedFirmwareAsset() {
  const asset = String(document.getElementById('fwAssetName')?.value || '').trim();
  return asset || 'firmware.bin';
}

function selectedFirmwareFsAsset() {
  const asset = String(document.getElementById('fwFsAssetName')?.value || '').trim();
  return asset || 'littlefs.bin';
}

function selectedFirmwareUrl() {
  return String(document.getElementById('fwUrl')?.value || '').trim();
}

function selectedFirmwareFsUrl() {
  return String(document.getElementById('fwFsUrl')?.value || '').trim();
}

function renderFirmwareReleaseOptions(selectedTag = '') {
  const select = document.getElementById('fwReleaseSelect');
  if (!select) return;
  select.innerHTML = '';
  firmwareReleases.forEach((release) => {
    const tag = String(release.tag || '').trim();
    if (!tag) return;
    const option = document.createElement('option');
    option.value = tag;
    option.textContent = release.publishedAt ? `${tag} (${release.publishedAt.slice(0, 10)})` : tag;
    select.appendChild(option);
  });
  if (!firmwareReleases.length) {
    const empty = document.createElement('option');
    empty.value = '';
    empty.textContent = t('fw_update_no_releases');
    select.appendChild(empty);
  }
  if (selectedTag) {
    select.value = selectedTag;
  } else if (firmwareReleases.length) {
    select.value = String(firmwareReleases[0].tag || '');
  }
}

async function saveFirmwareConfig() {
  const result = await req('/api/firmware/config', 'POST', {
    repo: selectedFirmwareRepo(),
    assetName: selectedFirmwareAsset(),
    filesystemAssetName: selectedFirmwareFsAsset(),
  });
  if (result.error) {
    setFirmwareStatus(`${t('fw_update_failed')}: ${result.error}`);
    return;
  }
  setFirmwareStatus('OTA config saved.');
}

async function loadFirmwareConfig() {
  const config = await req('/api/firmware/config');
  if (config.error) return;
  const repo = document.getElementById('fwRepo');
  const asset = document.getElementById('fwAssetName');
  const fsAsset = document.getElementById('fwFsAssetName');
  if (repo) repo.value = config.repo || '';
  if (asset) asset.value = config.assetName || 'firmware.bin';
  if (fsAsset) fsAsset.value = config.filesystemAssetName || 'littlefs.bin';
}

async function loadFirmwareReleases() {
  const repo = selectedFirmwareRepo();
  if (!repo) {
    setFirmwareStatus(t('fw_release_missing'));
    return;
  }
  setFirmwareStatus(t('fw_release_loading'));
  const payload = await req(`/api/firmware/releases?repo=${encodeURIComponent(repo)}`);
  if (payload.error) {
    setFirmwareStatus(`${t('fw_update_failed')}: ${payload.error}`);
    return;
  }
  firmwareReleases = Array.isArray(payload.releases) ? payload.releases : [];
  renderFirmwareReleaseOptions();
  setFirmwareStatus(`${t('fw_release_loaded')}: ${firmwareReleases.length}`);
}

async function updateFirmwareLatest() {
  const repo = selectedFirmwareRepo();
  if (!repo) {
    setFirmwareStatus(t('fw_release_missing'));
    return;
  }
  setFirmwareStatus('Downloading latest firmware...');
  const result = await req('/api/firmware/update', 'POST', {
    mode: 'latest',
    repo,
    assetName: selectedFirmwareAsset(),
    filesystemAssetName: selectedFirmwareFsAsset(),
  });
  if (result.error) {
    setFirmwareStatus(`${t('fw_update_failed')}: ${result.error}`);
    return;
  }
  setFirmwareStatus(result.message || t('fw_update_started'));
}

async function updateFirmwareSelected() {
  const repo = selectedFirmwareRepo();
  const tag = String(document.getElementById('fwReleaseSelect')?.value || '').trim();
  if (!repo) {
    setFirmwareStatus(t('fw_release_missing'));
    return;
  }
  if (!tag) {
    setFirmwareStatus(t('fw_update_no_releases'));
    return;
  }
  setFirmwareStatus(`Downloading ${tag}...`);
  const result = await req('/api/firmware/update', 'POST', {
    mode: 'tag',
    tag,
    repo,
    assetName: selectedFirmwareAsset(),
    filesystemAssetName: selectedFirmwareFsAsset(),
  });
  if (result.error) {
    setFirmwareStatus(`${t('fw_update_failed')}: ${result.error}`);
    return;
  }
  setFirmwareStatus(result.message || t('fw_update_started'));
}

async function updateFirmwareFromUrl() {
  const url = selectedFirmwareUrl();
  const filesystemUrl = selectedFirmwareFsUrl();
  if (!url || !filesystemUrl) {
    setFirmwareStatus(t('fw_update_url_missing'));
    return;
  }
  setFirmwareStatus(t('fw_update_url_started'));
  const result = await req('/api/firmware/update', 'POST', {
    mode: 'url',
    url,
    filesystemUrl,
    assetName: selectedFirmwareAsset(),
    filesystemAssetName: selectedFirmwareFsAsset(),
  });
  if (result.error) {
    setFirmwareStatus(`${t('fw_update_failed')}: ${result.error}`);
    return;
  }
  setFirmwareStatus(result.message || t('fw_update_started'));
}

async function saveReversePreference() {
  await req('/api/ui/preferences', 'POST', {
    reverse: document.getElementById('reverse').checked,
  });
}

async function saveUiLanguage() {
  const language = document.getElementById('uiLanguage').value;
  await req('/api/ui/preferences', 'POST', { language });
  applyLanguage(language);
}

async function saveSelectedMotor() {
  const nextMotor = Number(document.getElementById('motorSelect').value);
  if (!Number.isFinite(nextMotor)) return;
  currentMotorId = Math.max(0, Math.min(nextMotor, activeMotorCount - 1));
  await req('/api/ui/preferences', 'POST', { motorId: currentMotorId });
  await refreshSettings();
  await refresh();
}

async function loadUiPreferences() {
  try {
    const prefs = await req('/api/ui/preferences');
    ensureMotorSelectors(prefs.activeMotorCount || 1);
    if (typeof prefs.motorId === 'number') currentMotorId = Number(prefs.motorId);
    ensureMotorSelectors(prefs.activeMotorCount || 1);
    document.getElementById('motorSelect').value = String(currentMotorId);
    if (typeof prefs.reverse === 'boolean') {
      document.getElementById('reverse').checked = prefs.reverse;
    }
    applyLanguage(prefs.language || 'en');
  } catch (_) {
    applyLanguage('en');
  }
}

function growthProfileById(profileId) {
  const found = growthPrograms.find((p) => p.id === profileId);
  return found || growthPrograms[0] || DEFAULT_GROWTH_PROGRAMS[0];
}

function growthProgramName(program) {
  if (!program) return 'Program';
  if (currentLanguage === 'ru' && program.nameRu) return program.nameRu;
  return program.nameEn || program.nameRu || program.id || 'Program';
}

function growthPlantName(plant) {
  if (!plant) return '';
  if (currentLanguage === 'ru' && plant.nameRu) return plant.nameRu;
  return plant.nameEn || plant.nameRu || plant.id || '';
}

function growthFertilizerName(fertilizer) {
  if (!fertilizer) return '';
  if (currentLanguage === 'ru' && fertilizer.nameRu) return fertilizer.nameRu;
  return fertilizer.nameEn || fertilizer.nameRu || fertilizer.id || '';
}

function selectedGrowthProgram() {
  const plantId = String(document.getElementById('growthPlant')?.value || '');
  const fertilizerId = String(document.getElementById('growthFertilizer')?.value || '');
  const byPair = growthPrograms.find((program) => program.plantId === plantId && program.fertilizerId === fertilizerId);
  if (byPair) return byPair;
  const profileId = document.getElementById('growthProfile')?.value || '';
  return growthProfileById(profileId);
}

function renderGrowthPlantOptions() {
  const select = document.getElementById('growthPlant');
  if (!select) return;
  const previous = String(select.value || DEFAULT_GROWTH_PLANTS[0].id);
  select.innerHTML = DEFAULT_GROWTH_PLANTS.map((plant) =>
    `<option value="${escapeHtml(plant.id)}">${escapeHtml(growthPlantName(plant))}</option>`
  ).join('');
  select.value = DEFAULT_GROWTH_PLANTS.some((plant) => plant.id === previous) ? previous : DEFAULT_GROWTH_PLANTS[0].id;
}

function renderGrowthFertilizerOptions() {
  const select = document.getElementById('growthFertilizer');
  if (!select) return;
  const previous = String(select.value || DEFAULT_GROWTH_FERTILIZERS[0].id);
  select.innerHTML = DEFAULT_GROWTH_FERTILIZERS.map((fertilizer) =>
    `<option value="${escapeHtml(fertilizer.id)}">${escapeHtml(growthFertilizerName(fertilizer))}</option>`
  ).join('');
  select.value = DEFAULT_GROWTH_FERTILIZERS.some((fertilizer) => fertilizer.id === previous) ? previous : DEFAULT_GROWTH_FERTILIZERS[0].id;
}

function syncGrowthProfileFromSelectors() {
  const profileSelect = document.getElementById('growthProfile');
  if (!profileSelect) return;
  const plantId = String(document.getElementById('growthPlant')?.value || '');
  const fertilizerId = String(document.getElementById('growthFertilizer')?.value || '');
  if (!plantId || !fertilizerId) return;
  const matched = growthPrograms.find((program) => program.plantId === plantId && program.fertilizerId === fertilizerId);
  if (matched) profileSelect.value = matched.id;
}

function syncGrowthSelectorsFromProfile() {
  const profile = growthProfileById(document.getElementById('growthProfile')?.value || '');
  const plantSelect = document.getElementById('growthPlant');
  const fertilizerSelect = document.getElementById('growthFertilizer');
  if (plantSelect && profile?.plantId && DEFAULT_GROWTH_PLANTS.some((plant) => plant.id === profile.plantId)) {
    plantSelect.value = profile.plantId;
  }
  if (fertilizerSelect && profile?.fertilizerId && DEFAULT_GROWTH_FERTILIZERS.some((fertilizer) => fertilizer.id === profile.fertilizerId)) {
    fertilizerSelect.value = profile.fertilizerId;
  }
}

function normalizeGrowthPhase(raw, fallbackKey) {
  const key = String(raw?.key || fallbackKey || 'seedling');
  const feedingsPerWeek = Math.max(1, Math.min(7, Number(raw?.feedingsPerWeek || 1)));
  const phAdjustmentsPerWeek = Math.max(1, Math.min(7, Number(raw?.phAdjustmentsPerWeek || 1)));
  const nutrients = raw?.nutrients || {};
  const ph = raw?.ph || {};
  return {
    key,
    feedingsPerWeek,
    phAdjustmentsPerWeek,
    nutrients: {
      a: Math.max(0, Number(nutrients.a || 0)),
      b: Math.max(0, Number(nutrients.b || 0)),
      c: Math.max(0, Number(nutrients.c || 0)),
    },
    ph: {
      plus: Math.max(0, Number(ph.plus || 0)),
      minus: Math.max(0, Number(ph.minus || 0)),
    },
  };
}

function normalizeGrowthProgram(raw, index = 0) {
  const fallback = DEFAULT_GROWTH_PROGRAMS[index % DEFAULT_GROWTH_PROGRAMS.length];
  const id = String(raw?.id || fallback.id || `program-${index + 1}`).toLowerCase().replace(/[^a-z0-9-_]/g, '-');
  const phasesRaw = Array.isArray(raw?.phases) ? raw.phases : fallback.phases;
  return {
    id,
    fertilizerId: String(raw?.fertilizerId || fallback.fertilizerId || ''),
    fertilizerNameEn: String(raw?.fertilizerNameEn || fallback.fertilizerNameEn || ''),
    fertilizerNameRu: String(raw?.fertilizerNameRu || fallback.fertilizerNameRu || raw?.fertilizerNameEn || fallback.fertilizerNameEn || ''),
    plantId: String(raw?.plantId || fallback.plantId || ''),
    plantNameEn: String(raw?.plantNameEn || fallback.plantNameEn || ''),
    plantNameRu: String(raw?.plantNameRu || fallback.plantNameRu || raw?.plantNameEn || fallback.plantNameEn || ''),
    nameEn: String(raw?.nameEn || raw?.name || fallback.nameEn || id),
    nameRu: String(raw?.nameRu || raw?.name || fallback.nameRu || fallback.nameEn || id),
    source: String(raw?.source || 'imported'),
    phases: phasesRaw.map((phase, phaseIndex) =>
      normalizeGrowthPhase(phase, GROWTH_PHASE_KEYS[phaseIndex] || 'seedling')
    ),
  };
}

function migrateGrowthProgramsPayload(payload) {
  if (payload && payload.schema === GROWTH_SCHEMA && Number(payload.schemaVersion) === GROWTH_SCHEMA_VERSION && Array.isArray(payload.programs)) {
    return payload.programs.map((program, idx) => normalizeGrowthProgram(program, idx));
  }
  if (Array.isArray(payload)) {
    return payload.map((program, idx) => normalizeGrowthProgram(program, idx));
  }
  if (payload && typeof payload === 'object') {
    const values = Object.entries(payload).filter(([, value]) => value && typeof value === 'object' && Array.isArray(value.phases));
    if (values.length) {
      return values.map(([id, program], idx) => normalizeGrowthProgram({ ...program, id }, idx));
    }
  }
  throw new Error('unsupported growth programs format');
}

function saveGrowthProgramsToStorage() {
  try {
    localStorage.setItem(GROWTH_STORAGE_KEY, JSON.stringify({
      schema: GROWTH_SCHEMA,
      schemaVersion: GROWTH_SCHEMA_VERSION,
      exportedAt: new Date().toISOString(),
      programs: growthPrograms,
    }));
  } catch (_) {
  }
}

function loadGrowthProgramsFromStorage() {
  try {
    const raw = localStorage.getItem(GROWTH_STORAGE_KEY);
    if (!raw) return;
    const parsed = JSON.parse(raw);
    const migrated = migrateGrowthProgramsPayload(parsed);
    if (Array.isArray(migrated) && migrated.length) growthPrograms = migrated;
  } catch (_) {
    growthPrograms = JSON.parse(JSON.stringify(DEFAULT_GROWTH_PROGRAMS));
  }
}

function setGrowthImportStatus(messageKeyOrText, extra = '') {
  const node = document.getElementById('growthImportStatus');
  if (!node) return;
  const base = I18N.en[messageKeyOrText] || I18N.ru[messageKeyOrText] ? t(messageKeyOrText) : messageKeyOrText;
  node.textContent = extra ? `${base}: ${extra}` : base;
}

function exportGrowthPrograms() {
  const payload = {
    schema: GROWTH_SCHEMA,
    schemaVersion: GROWTH_SCHEMA_VERSION,
    exportedAt: new Date().toISOString(),
    programs: growthPrograms,
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = `growth-programs-v${GROWTH_SCHEMA_VERSION}.json`;
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(link.href);
  setGrowthImportStatus('growth_export_ok');
}

function openGrowthImport() {
  const input = document.getElementById('growthImportFile');
  if (!input) return;
  input.value = '';
  input.click();
}

async function importGrowthProgramsFromFile(file) {
  const text = await file.text();
  const parsed = JSON.parse(text);
  const migrated = migrateGrowthProgramsPayload(parsed);
  if (!Array.isArray(migrated) || migrated.length === 0) throw new Error('no programs');
  growthPrograms = migrated;
  saveGrowthProgramsToStorage();
  renderGrowthPlantOptions();
  renderGrowthFertilizerOptions();
  renderGrowthProfileOptions();
  syncGrowthSelectorsFromProfile();
  renderGrowthPhaseOptions();
  calculateGrowthProgram();
  setGrowthImportStatus('growth_import_ok', String(growthPrograms.length));
}

function selectedGrowthPhase(profile) {
  const phaseKey = document.getElementById('growthPhase')?.value || '';
  const found = profile.phases.find((phase) => phase.key === phaseKey);
  return found || profile.phases[0];
}

function renderGrowthProfileOptions() {
  const profileSelect = document.getElementById('growthProfile');
  if (!profileSelect) return;
  if (!growthPrograms.length) growthPrograms = JSON.parse(JSON.stringify(DEFAULT_GROWTH_PROGRAMS));
  const previous = profileSelect.value || growthPrograms[0].id;
  profileSelect.innerHTML = growthPrograms.map((program) =>
    `<option value="${escapeHtml(program.id)}">${escapeHtml(growthProgramName(program))}</option>`
  ).join('');
  profileSelect.value = growthPrograms.some((program) => program.id === previous) ? previous : growthPrograms[0].id;
}

function renderGrowthPhaseOptions() {
  const phaseSelect = document.getElementById('growthPhase');
  if (!phaseSelect) return;
  const profile = selectedGrowthProgram();
  if (!profile || !Array.isArray(profile.phases) || profile.phases.length === 0) {
    phaseSelect.innerHTML = '';
    return;
  }
  const previous = phaseSelect.value || '';
  phaseSelect.innerHTML = profile.phases.map((phase) =>
    `<option value="${phase.key}">${escapeHtml(t(`growth_phase_${phase.key}`))}</option>`
  ).join('');
  phaseSelect.value = profile.phases.some((phase) => phase.key === previous) ? previous : profile.phases[0].key;
}

function updateGrowthTabVisibility() {
  const tab = document.getElementById('tabGrowth');
  const btn = document.getElementById('tabGrowthBtn');
  if (!tab || !btn) return;
  btn.style.display = growthProgramEnabled ? '' : 'none';
  if (!growthProgramEnabled && tab.classList.contains('active')) showTab('control');
}

function weekdaysMaskForFrequency(timesPerWeek) {
  const count = Math.max(1, Math.min(7, Math.round(Number(timesPerWeek) || 1)));
  if (count >= 7) return 0x7F;
  const presets = {
    1: [2],         // Wed
    2: [0, 3],      // Mon, Thu
    3: [0, 2, 4],   // Mon, Wed, Fri
    4: [0, 1, 3, 5],// Mon, Tue, Thu, Sat
    5: [0, 1, 2, 4, 6], // Mon, Tue, Wed, Fri, Sun
    6: [0, 1, 2, 3, 4, 5], // Mon-Sat
  };
  const days = presets[count] || presets[3];
  return days.reduce((mask, day) => mask | (1 << day), 0);
}

function calculateGrowthProgram() {
  const profile = selectedGrowthProgram();
  if (!profile) return;
  const phase = selectedGrowthPhase(profile);
  const waterL = Math.max(1, Number(document.getElementById('growthWaterVolumeL')?.value || 0));
  const feedings = Math.max(1, Number(phase.feedingsPerWeek || 1));
  const phAdjustments = phRegulationEnabled ? Math.max(1, Number(phase.phAdjustmentsPerWeek || 1)) : 0;
  const phScale = waterL / 10.0;
  const items = [
    {
      key: 'growth_pump_nutrient_a',
      weeklyMl: phase.nutrients.a * waterL * feedings,
      perDoseMl: phase.nutrients.a * waterL,
      frequency: feedings,
      unit: 'growth_feedings_week',
    },
    {
      key: 'growth_pump_nutrient_b',
      weeklyMl: phase.nutrients.b * waterL * feedings,
      perDoseMl: phase.nutrients.b * waterL,
      frequency: feedings,
      unit: 'growth_feedings_week',
    },
    {
      key: 'growth_pump_nutrient_c',
      weeklyMl: phase.nutrients.c * waterL * feedings,
      perDoseMl: phase.nutrients.c * waterL,
      frequency: feedings,
      unit: 'growth_feedings_week',
    },
  ];
  if (phRegulationEnabled) {
    items.unshift(
      {
        key: 'growth_pump_ph_minus',
        weeklyMl: phase.ph.minus * phScale * phAdjustments,
        perDoseMl: phase.ph.minus * phScale,
        frequency: phAdjustments,
        unit: 'growth_ph_adjustments_week',
      },
      {
        key: 'growth_pump_ph_plus',
        weeklyMl: phase.ph.plus * phScale * phAdjustments,
        perDoseMl: phase.ph.plus * phScale,
        frequency: phAdjustments,
        unit: 'growth_ph_adjustments_week',
      },
    );
  }

  const list = document.getElementById('growthResultList');
  if (list) {
    list.innerHTML = items.map((item) => `
      <div class="schedule-item">
        <div class="growth-main">
          <b>${escapeHtml(t(item.key))}</b>
          <span class="growth-dose">${t('growth_weekly_total')}: ${item.weeklyMl.toFixed(1)} ml</span>
          <span class="growth-dose">${t('growth_per_dose')}: ${item.perDoseMl.toFixed(1)} ml</span>
          <span class="schedule-tag">${item.frequency} ${t(item.unit)}</span>
        </div>
      </div>
    `).join('');
  }

  const summary = document.getElementById('growthScheduleSummary');
  if (summary) {
    const fertilizerName = currentLanguage === 'ru'
      ? (profile.fertilizerNameRu || profile.fertilizerNameEn || '')
      : (profile.fertilizerNameEn || profile.fertilizerNameRu || '');
    const plantName = currentLanguage === 'ru'
      ? (profile.plantNameRu || profile.plantNameEn || '')
      : (profile.plantNameEn || profile.plantNameRu || '');
    const pairLabel = (fertilizerName && plantName) ? `${fertilizerName} / ${plantName}` : growthProgramName(profile);
    const frequencyLine = phRegulationEnabled
      ? `${feedings} ${t('growth_feedings_week')}, ${phAdjustments} ${t('growth_ph_adjustments_week')}`
      : `${feedings} ${t('growth_feedings_week')}, ${t('growth_schedule_ph_off')}`;
    summary.textContent = `${pairLabel}: ${frequencyLine}`;
  }
}

async function createGrowthSchedule() {
  const profile = selectedGrowthProgram();
  if (!profile) return;
  if (!growthScheduler || typeof growthScheduler.generateGrowthScheduleEntries !== 'function') {
    const summary = document.getElementById('growthScheduleSummary');
    if (summary) summary.textContent = 'Growth scheduler is unavailable.';
    return;
  }
  const programName = growthProgramName(profile);
  const phase = selectedGrowthPhase(profile);
  const waterL = Math.max(1, Number(document.getElementById('growthWaterVolumeL')?.value || 0));
  const feedings = Math.max(1, Number(phase.feedingsPerWeek || 1));
  const phAdjustments = phRegulationEnabled ? Math.max(1, Number(phase.phAdjustmentsPerWeek || 1)) : 0;
  const nutrientHour = document.getElementById('growthNutrientHour')?.value;
  const nutrientMinute = document.getElementById('growthNutrientMinute')?.value;
  const phHour = document.getElementById('growthPhHour')?.value;
  const phMinute = document.getElementById('growthPhMinute')?.value;
  const pauseMinutes = document.getElementById('growthPauseMinutes')?.value;
  const nutrientMask = weekdaysMaskForFrequency(feedings);
  const phMask = weekdaysMaskForFrequency(phAdjustments);

  const generatedResult = growthScheduler.generateGrowthScheduleEntries({
    programName,
    activeMotorCount,
    phase,
    waterL,
    phRegulationEnabled,
    nutrientHour,
    nutrientMinute,
    nutrientMask,
    phHour,
    phMinute,
    phMask,
    pauseMinutes,
    nameResolver: (key) => t(key),
  });
  const generated = generatedResult.entries;
  const remapped = generatedResult.remapped;

  scheduleEntries = generated;
  editingScheduleIndex = -1;
  renderScheduleEntries();
  await saveSchedule();
  showTab('schedule');
  const summary = document.getElementById('growthScheduleSummary');
  if (summary) {
    const warning = remapped > 0 ? `, ${t('growth_schedule_motors_warning')}` : '';
    summary.textContent = `${t('growth_schedule_created')}: ${generated.length}${warning}`;
  }
}

function applyLanguage(lang) {
  currentLanguage = (lang === 'ru') ? 'ru' : 'en';
  document.documentElement.lang = currentLanguage;
  Object.keys(I18N.en).forEach((key) => {
    if (key === 'days') return;
    const el = document.getElementById(key);
    if (el) el.textContent = t(key);
  });
  const dayLabels = t('days');
  for (let i = 0; i < 7; i += 1) {
    const el = document.getElementById(`schDay${i}Label`);
    if (el) el.textContent = dayLabels[i];
  }
  const haEn = document.getElementById('haContentEn');
  const haRu = document.getElementById('haContentRu');
  if (haEn && haRu) {
    haEn.style.display = (currentLanguage === 'en') ? '' : 'none';
    haRu.style.display = (currentLanguage === 'ru') ? '' : 'none';
  }
  const uiLanguage = document.getElementById('uiLanguage');
  if (uiLanguage) uiLanguage.value = currentLanguage;
  ensureMotorSelectors(activeMotorCount);
  renderScheduleEntries();
  updateScheduleActionButtons();
  renderGrowthPlantOptions();
  renderGrowthFertilizerOptions();
  renderGrowthProfileOptions();
  syncGrowthSelectorsFromProfile();
  renderGrowthPhaseOptions();
  calculateGrowthProgram();
  setGrowthImportStatus('growth_import_status_idle');
  updateGrowthTabVisibility();
  renderMotorsOverview(lastState?.motors || []);
  if (lastState) {
    document.getElementById('modeName').textContent = uiModeName(lastState);
    document.getElementById('directionName').textContent = uiDirectionName(lastState);
    document.getElementById('modeChip').textContent = uiModeName(lastState);
    document.getElementById('dirChip').textContent = uiDirectionName(lastState);
  }
}

document.addEventListener('DOMContentLoaded', () => {
  loadGrowthProgramsFromStorage();
  populateTimezoneSelect(0);
  clearScheduleForm();
  updateScheduleActionButtons();
  renderGrowthPlantOptions();
  renderGrowthFertilizerOptions();
  renderGrowthProfileOptions();
  syncGrowthSelectorsFromProfile();
  renderGrowthPhaseOptions();
  calculateGrowthProgram();
  renderFirmwareReleaseOptions();
  document.getElementById('reverse').addEventListener('change', saveReversePreference);
  document.getElementById('uiLanguage').addEventListener('change', () => {
    applyLanguage(document.getElementById('uiLanguage').value);
  });
  document.getElementById('motorSelect').addEventListener('change', saveSelectedMotor);
  document.getElementById('tzOffsetMinutes').addEventListener('change', () => {
    currentTzOffsetMinutes = Number(document.getElementById('tzOffsetMinutes').value || 0);
  });
  document.getElementById('growthPlant').addEventListener('change', () => {
    syncGrowthProfileFromSelectors();
    renderGrowthPhaseOptions();
    calculateGrowthProgram();
  });
  document.getElementById('growthFertilizer').addEventListener('change', () => {
    syncGrowthProfileFromSelectors();
    renderGrowthPhaseOptions();
    calculateGrowthProgram();
  });
  document.getElementById('growthProfile').addEventListener('change', () => {
    syncGrowthSelectorsFromProfile();
    renderGrowthPhaseOptions();
    calculateGrowthProgram();
  });
  document.getElementById('growthPhase').addEventListener('change', calculateGrowthProgram);
  document.getElementById('growthWaterVolumeL').addEventListener('input', calculateGrowthProgram);
  document.getElementById('growthNutrientHour').addEventListener('input', calculateGrowthProgram);
  document.getElementById('growthNutrientMinute').addEventListener('input', calculateGrowthProgram);
  document.getElementById('growthPhHour').addEventListener('input', calculateGrowthProgram);
  document.getElementById('growthPhMinute').addEventListener('input', calculateGrowthProgram);
  document.getElementById('growthPauseMinutes').addEventListener('input', calculateGrowthProgram);
  document.getElementById('phRegulationEnabled').addEventListener('change', () => {
    phRegulationEnabled = document.getElementById('phRegulationEnabled').checked;
    calculateGrowthProgram();
  });
  document.getElementById('growthImportFile').addEventListener('change', async (event) => {
    const file = event.target.files && event.target.files[0];
    if (!file) return;
    try {
      await importGrowthProgramsFromFile(file);
    } catch (error) {
      setGrowthImportStatus('growth_import_err', error && error.message ? error.message : 'invalid json');
    }
  });
});

function showTab(name) {
  document.getElementById('tabControl').classList.remove('active');
  document.getElementById('tabCal').classList.remove('active');
  document.getElementById('tabSchedule').classList.remove('active');
  document.getElementById('tabGrowth').classList.remove('active');
  document.getElementById('tabHa').classList.remove('active');
  document.getElementById('tabControlBtn').classList.remove('active');
  document.getElementById('tabCalBtn').classList.remove('active');
  document.getElementById('tabScheduleBtn').classList.remove('active');
  document.getElementById('tabGrowthBtn').classList.remove('active');
  document.getElementById('tabHaBtn').classList.remove('active');
  if (name === 'control') {
    document.getElementById('tabControl').classList.add('active');
    document.getElementById('tabControlBtn').classList.add('active');
  } else if (name === 'cal') {
    document.getElementById('tabCal').classList.add('active');
    document.getElementById('tabCalBtn').classList.add('active');
  } else if (name === 'schedule') {
    document.getElementById('tabSchedule').classList.add('active');
    document.getElementById('tabScheduleBtn').classList.add('active');
  } else if (name === 'growth') {
    if (!growthProgramEnabled) {
      showTab('control');
      return;
    }
    document.getElementById('tabGrowth').classList.add('active');
    document.getElementById('tabGrowthBtn').classList.add('active');
  } else {
    document.getElementById('tabHa').classList.add('active');
    document.getElementById('tabHaBtn').classList.add('active');
  }
}

async function startFlow() {
  const reverse = document.getElementById('reverse').checked;
  await post('/api/flow', {
    motorId: currentMotorId,
    litersPerHour: Number(document.getElementById('flowLph').value),
    reverse,
  });
}

async function startDosing() {
  const reverse = document.getElementById('reverse').checked;
  await post('/api/dosing', {
    motorId: currentMotorId,
    volumeMl: Number(document.getElementById('dosingMl').value),
    reverse,
  });
}

async function stopPump() {
  await post('/api/stop', { motorId: currentMotorId });
}

async function saveSettings() {
  normalizeAliases(activeMotorCount);
  for (let i = 0; i < activeMotorCount; i += 1) {
    const input = document.getElementById(`motorAlias${i}`);
    if (input) {
      const trimmed = String(input.value || '').trim();
      motorAliases[i] = trimmed || `${t('motor_card')} ${i}`;
    }
  }
  await post('/api/settings', {
    motorId: currentMotorId,
    mlPerRevCw: Number(document.getElementById('mlCw').value),
    mlPerRevCcw: Number(document.getElementById('mlCcw').value),
    dosingFlowLph: Number(document.getElementById('dFlow').value),
    maxFlowLph: Number(document.getElementById('maxFlowLph').value),
    ntpServer: document.getElementById('ntpServer').value.trim(),
    tzOffsetMinutes: Number(document.getElementById('tzOffsetMinutes').value || 0),
    growthProgramEnabled: document.getElementById('growthProgramEnabled').checked,
    phRegulationEnabled: document.getElementById('phRegulationEnabled').checked,
    motorAliases,
    expansion: {
      enabled: document.getElementById('expansionEnabled').checked,
      interface: document.getElementById('expansionInterface').value,
      motorCount: Number(document.getElementById('expansionMotorCount').value),
    },
  });
  await refreshSettings();
}

async function saveSecurity() {
  await req('/api/ui/security', 'POST', {
    enabled: document.getElementById('authEnabled').checked,
    username: document.getElementById('authUser').value || 'admin',
    password: document.getElementById('authPass').value || 'admin',
  });
  await refreshSecurity();
}

async function runCalibration() {
  await post('/api/calibration/run', {
    motorId: currentMotorId,
    direction: document.getElementById('dir').value,
    revolutions: Number(document.getElementById('revs').value),
  });
}

async function applyCalibration() {
  await post('/api/calibration/apply', {
    motorId: currentMotorId,
    direction: document.getElementById('dir').value,
    revolutions: Number(document.getElementById('revs').value),
    measuredMl: Number(document.getElementById('measuredMl').value),
  });
}

function localizeMode(modeName, running) {
  if (!running) return t('mode_idle');
  if (modeName === 'dosing') return t('mode_dosing');
  return t('mode_flow_lph');
}

function localizeDirection(direction, running) {
  if (!running) return t('dir_none');
  if (direction === 'reverse') return t('dir_reverse');
  return t('dir_forward');
}

function uiModeName(s) {
  return localizeMode(s.modeName, s.running);
}

function uiDirectionName(s) {
  return localizeDirection(s.direction, s.running);
}

function renderMotorsOverview(motors) {
  const root = document.getElementById('motorsOverview');
  if (!root) return;
  root.innerHTML = (motors || []).map((m) => `
    <div class="schedule-item">
      <div class="schedule-main">
        <span class="schedule-tag">${escapeHtml(motorLabel(Number(m.motorId || 0)))}</span>
        <span>${uiModeName(m)}</span>
        <span>${uiDirectionName(m)}</span>
        <span>${Number(Math.abs(m.flowLph || 0)).toFixed(2)} L/h</span>
        <span>${Number(m.dosingRemainingMl || 0).toFixed(0)} ml</span>
      </div>
    </div>
  `).join('');
}

async function refresh() {
  const s = await req(`/api/state?motorId=${currentMotorId}`);
  lastState = s;
  syncAliasesFromMotors(s.motors || []);
  if (s.motorAlias && Number.isFinite(Number(s.motorId))) {
    motorAliases[Number(s.motorId)] = String(s.motorAlias);
  }
  if (typeof s.motorId === 'number') currentMotorId = Number(s.motorId);
  ensureMotorSelectors(s.activeMotorCount || 1);
  document.getElementById('motorSelect').value = String(currentMotorId);
  if (s.uiLanguage && s.uiLanguage !== currentLanguage) applyLanguage(s.uiLanguage);
  document.getElementById('state').textContent = JSON.stringify(s, null, 2);

  const flow = document.getElementById('flowLph');
  const cw = document.getElementById('mlCw');
  const ccw = document.getElementById('mlCcw');
  const dflow = document.getElementById('dFlow');
  if (document.activeElement !== cw) cw.value = s.mlPerRevCw;
  if (document.activeElement !== ccw) ccw.value = s.mlPerRevCcw;
  if (document.activeElement !== dflow && Number(s.dosingFlowLph || 0) > 0) dflow.value = Number(s.dosingFlowLph).toFixed(2);
  const targetFlowAbs = Number(Math.abs(s.targetFlowLph || 0));
  if (document.activeElement !== flow && s.modeName === 'flow_lph' && targetFlowAbs > 0.0001) flow.value = targetFlowAbs.toFixed(2);

  document.getElementById('reverse').checked = s.running ? (s.direction === 'reverse') : !!s.preferredReverse;
  const m = uiModeName(s);
  const d = uiDirectionName(s);
  document.getElementById('modeName').textContent = m;
  document.getElementById('directionName').textContent = d;
  document.getElementById('currentFlowLph').textContent = Number(Math.abs(s.flowLph || 0)).toFixed(2);
  document.getElementById('totalPumpedL').textContent = Number(s.totalPumpedL || 0).toFixed(3);
  document.getElementById('dosingLeftMl').textContent = Number(s.dosingRemainingMl || 0).toFixed(0);
  document.getElementById('modeChip').textContent = m;
  document.getElementById('dirChip').textContent = d;
  document.getElementById('timeChip').textContent = s.time || '-';
  document.getElementById('stopBtn').disabled = !s.running;
  renderMotorsOverview(s.motors || []);
}

function formatScheduleTime(hour, minute) {
  const h = String(Number(hour) || 0).padStart(2, '0');
  const m = String(Number(minute) || 0).padStart(2, '0');
  return `${h}:${m}`;
}

function getWeekdaysMaskFromForm() {
  let mask = 0;
  for (let i = 0; i < 7; i += 1) {
    const cb = document.getElementById(`schDay${i}`);
    if (cb && cb.checked) mask |= (1 << i);
  }
  return mask || 0x7F;
}

function formatWeekdays(mask) {
  const safeMask = Number(mask ?? 0x7F) & 0x7F;
  if (safeMask === 0x7F) return t('sched_daily');
  return t('days').filter((_, i) => (safeMask & (1 << i)) !== 0).join(', ');
}

function defaultScheduleName(index) {
  return `${t('schedule_rule')} ${index + 1}`;
}

function updateScheduleActionButtons() {
  const addBtn = document.getElementById('addScheduleBtn');
  const cancelBtn = document.getElementById('cancelScheduleEditBtn');
  if (addBtn) addBtn.textContent = editingScheduleIndex >= 0 ? t('schedule_update') : t('addScheduleBtn');
  if (cancelBtn) cancelBtn.style.display = editingScheduleIndex >= 0 ? '' : 'none';
}

function setWeekdaysMaskToForm(mask) {
  const safeMask = Number(mask ?? 0x7F) & 0x7F;
  for (let i = 0; i < 7; i += 1) {
    const cb = document.getElementById(`schDay${i}`);
    if (cb) cb.checked = (safeMask & (1 << i)) !== 0;
  }
}

function clearScheduleForm() {
  document.getElementById('schName').value = '';
  document.getElementById('schHour').value = '9';
  document.getElementById('schMinute').value = '0';
  document.getElementById('schVolumeMl').value = '50';
  document.getElementById('schReverse').checked = false;
  document.getElementById('schEnabled').checked = true;
  document.getElementById('schMotorId').value = String(currentMotorId);
  setWeekdaysMaskToForm(0x7F);
}

function renderScheduleEntries() {
  const list = document.getElementById('scheduleList');
  if (!scheduleEntries.length) {
    list.innerHTML = `<div class="schedule-empty">${t('schedule_empty')}</div>`;
    updateScheduleActionButtons();
    return;
  }
  list.innerHTML = scheduleEntries.map((entry, idx) => `
    <div class="schedule-item">
      <div class="schedule-main">
        <span class="schedule-tag">${entry.enabled ? t('sched_enabled') : t('sched_disabled')}</span>
        <b>${escapeHtml(entry.name || defaultScheduleName(idx))}</b>
        <b>${formatScheduleTime(entry.hour, entry.minute)}</b>
        <span>${escapeHtml(motorLabel(Number(entry.motorId || 0)))}</span>
        <span>${Number(entry.volumeMl || 0).toFixed(0)} ml</span>
        <span>${entry.reverse ? t('sched_reverse') : t('sched_forward')}</span>
        <span>${formatWeekdays(entry.weekdaysMask)}</span>
      </div>
      <button class="btn ghost" onclick="editScheduleEntry(${idx})">${t('schedule_edit')}</button>
      <button class="btn stop" onclick="removeScheduleEntry(${idx})">${t('schedule_delete')}</button>
    </div>
  `).join('');
  updateScheduleActionButtons();
}

async function removeScheduleEntry(index) {
  if (index < 0 || index >= scheduleEntries.length) return;
  scheduleEntries.splice(index, 1);
  if (editingScheduleIndex === index) {
    editingScheduleIndex = -1;
    clearScheduleForm();
  } else if (editingScheduleIndex > index) {
    editingScheduleIndex -= 1;
  }
  renderScheduleEntries();
  await saveSchedule();
}

function editScheduleEntry(index) {
  if (index < 0 || index >= scheduleEntries.length) return;
  const entry = scheduleEntries[index];
  editingScheduleIndex = index;
  document.getElementById('schName').value = entry.name || '';
  document.getElementById('schMotorId').value = String(Number(entry.motorId || 0));
  document.getElementById('schEnabled').checked = !!entry.enabled;
  document.getElementById('schHour').value = String(Number(entry.hour || 0));
  document.getElementById('schMinute').value = String(Number(entry.minute || 0));
  document.getElementById('schVolumeMl').value = String(Number(entry.volumeMl || 0));
  document.getElementById('schReverse').checked = !!entry.reverse;
  setWeekdaysMaskToForm(entry.weekdaysMask);
  updateScheduleActionButtons();
}

function cancelScheduleEdit() {
  editingScheduleIndex = -1;
  clearScheduleForm();
  updateScheduleActionButtons();
}

async function addScheduleEntry() {
  const utf8Trim = (value, maxBytes) => {
    const text = String(value || '').trim();
    if (!text) return '';
    if (growthScheduler && typeof growthScheduler.truncateUtf8 === 'function') {
      return growthScheduler.truncateUtf8(text, maxBytes);
    }
    return text.slice(0, maxBytes);
  };
  const nextEntry = {
    name: utf8Trim(document.getElementById('schName').value || '', 32),
    motorId: Number(document.getElementById('schMotorId').value || 0),
    enabled: document.getElementById('schEnabled').checked,
    hour: Number(document.getElementById('schHour').value),
    minute: Number(document.getElementById('schMinute').value),
    volumeMl: Number(document.getElementById('schVolumeMl').value),
    reverse: document.getElementById('schReverse').checked,
    weekdaysMask: getWeekdaysMaskFromForm(),
  };
  if (!nextEntry.name) {
    const indexHint = editingScheduleIndex >= 0 ? editingScheduleIndex : scheduleEntries.length;
    nextEntry.name = utf8Trim(defaultScheduleName(indexHint), 32);
  }
  if (editingScheduleIndex >= 0 && editingScheduleIndex < scheduleEntries.length) {
    scheduleEntries[editingScheduleIndex] = nextEntry;
  } else {
    scheduleEntries.push(nextEntry);
  }
  editingScheduleIndex = -1;
  clearScheduleForm();
  renderScheduleEntries();
  await saveSchedule();
}

async function loadSchedule() {
  const s = await req('/api/schedule');
  if (typeof s.tzOffsetMinutes === 'number') {
    currentTzOffsetMinutes = Number(s.tzOffsetMinutes);
    populateTimezoneSelect(currentTzOffsetMinutes);
  }
  scheduleEntries = (s.entries || []).map((entry) => ({
    ...entry,
    name: String(entry.name || '').trim(),
    motorId: Number(entry.motorId || 0),
    weekdaysMask: Number(entry.weekdaysMask ?? 0x7F) & 0x7F,
  }));
  editingScheduleIndex = -1;
  clearScheduleForm();
  renderScheduleEntries();
}

async function saveSchedule() {
  await post('/api/schedule', {
    tzOffsetMinutes: currentTzOffsetMinutes,
    entries: scheduleEntries,
  });
  await loadSchedule();
}

async function refreshSecurity() {
  try {
    const sec = await req('/api/ui/security');
    document.getElementById('authEnabled').checked = !!sec.enabled;
    if (sec.username) document.getElementById('authUser').value = sec.username;
  } catch (_) {
  }
}

async function refreshSettings() {
  try {
    const settings = await req(`/api/settings?motorId=${currentMotorId}`);
    const ntpServer = document.getElementById('ntpServer');
    const maxFlowLph = document.getElementById('maxFlowLph');
    if (ntpServer && document.activeElement !== ntpServer) ntpServer.value = settings.ntpServer || 'time.google.com';
    if (maxFlowLph && document.activeElement !== maxFlowLph) maxFlowLph.value = Number(settings.maxFlowLph || 30).toFixed(1);
    if (typeof settings.tzOffsetMinutes === 'number') {
      currentTzOffsetMinutes = Number(settings.tzOffsetMinutes);
      populateTimezoneSelect(currentTzOffsetMinutes);
    }
    growthProgramEnabled = !!settings.growthProgramEnabled;
    document.getElementById('growthProgramEnabled').checked = growthProgramEnabled;
    phRegulationEnabled = !!settings.phRegulationEnabled;
    document.getElementById('phRegulationEnabled').checked = phRegulationEnabled;
    calculateGrowthProgram();
    updateGrowthTabVisibility();
    if (Array.isArray(settings.motorAliases)) {
      motorAliases = settings.motorAliases.map((a, i) => {
        const alias = String(a || '').trim();
        return alias || `${t('motor_card')} ${i}`;
      });
    }
    if (settings.expansion) {
      document.getElementById('expansionEnabled').checked = !!settings.expansion.enabled;
      document.getElementById('expansionInterface').value = settings.expansion.interface || 'i2c';
      document.getElementById('expansionMotorCount').value = Number(settings.expansion.motorCount || 0);
      ensureMotorSelectors((settings.expansion.enabled ? Number(settings.expansion.motorCount || 0) : 0) + 1);
    }
    if (settings.firmwareUpdate) {
      const repo = document.getElementById('fwRepo');
      const asset = document.getElementById('fwAssetName');
      const fsAsset = document.getElementById('fwFsAssetName');
      if (repo && document.activeElement !== repo) repo.value = settings.firmwareUpdate.repo || 'dslimp/peristaltic-pump';
      if (asset && document.activeElement !== asset) asset.value = settings.firmwareUpdate.assetName || 'firmware.bin';
      if (fsAsset && document.activeElement !== fsAsset) fsAsset.value = settings.firmwareUpdate.filesystemAssetName || 'littlefs.bin';
    } else {
      const repo = document.getElementById('fwRepo');
      if (repo && document.activeElement !== repo) repo.value = 'dslimp/peristaltic-pump';
    }
  } catch (_) {
  }
}

setInterval(refresh, 2000);
loadUiPreferences();
loadFirmwareConfig();
refresh();
refreshSecurity();
refreshSettings();
loadSchedule();
