// Local storage keys.
const storageKeys = {
    ip: "vg_device_ip",
    tz: "vg_timezone",
    mode: "vg_time_mode",
    manual: "vg_manual_time",
    theme: "vg_theme",
    language: "vg_language",
    port: "vg_port",
    live: "vg_live",
    liveRateEnabled: "vg_live_rate_enabled",
    liveInterval: "vg_live_interval"
};

const DISPLAY_LOCALE = "de-DE";
const DEFAULT_TIMEZONE = "Europe/Berlin";

// Main form and action elements.
const ipInputEl = document.getElementById("ipInput");
const timezoneInputEl = document.getElementById("timezoneInput");
const timeModeEl = document.getElementById("timeMode");
const manualTimeEl = document.getElementById("manualTime");
const connectBtnEl = document.getElementById("connectBtn");
const refreshControllerBtnEl = document.getElementById("refreshControllerBtn");
const syncBtnEl = document.getElementById("syncBtn");
const autoNowTextEl = document.getElementById("autoNowText");
const themeToggleEl = document.getElementById("themeToggle");
const languageToggleEl = document.getElementById("languageToggle");
const resetButtonEl = document.getElementById("resetButton");
const advancedToggleEl = document.getElementById("advancedToggle");
const advancedContentEl = document.getElementById("advancedContent");
const advancedChevronEl = document.getElementById("advancedChevron");
const portEnabledEl = document.getElementById("portEnabled");
const portInputEl = document.getElementById("portInput");
const liveRateEnabledEl = document.getElementById("liveRateEnabled");
const liveRateEnabledLabelEl = document.getElementById("liveRateEnabledLabel");
const liveIntervalLabelEl = document.getElementById("liveIntervalLabel");
const liveIntervalInputEl = document.getElementById("liveIntervalInput");
const liveStatusToggleEl = document.getElementById("liveStatusToggle");
const liveStatusToggleTextEl = document.getElementById("liveStatusToggleText");
const liveStatusTitleEl = document.getElementById("liveStatusTitle");
const globalStatusEl = document.getElementById("globalStatus");

// Headline and label elements.
const titleTextEl = document.getElementById("titleText");
const subtitleTextEl = document.getElementById("subtitleText");
const settingsTitleEl = document.getElementById("settingsTitle");
const advancedTitleEl = document.getElementById("advancedTitle");
const timeTitleEl = document.getElementById("timeTitle");
const ipLabelEl = document.getElementById("ipLabel");
const timezoneLabelEl = document.getElementById("timezoneLabel");
const portLabelEl = document.getElementById("portLabel");
const timeModeLabelEl = document.getElementById("timeModeLabel");
const manualTimeLabelEl = document.getElementById("manualTimeLabel");

// Connection and controller info elements.
const connectionDotEl = document.getElementById("connectionDot");
const connectionTextEl = document.getElementById("connectionText");
const ctrlTimeEl = document.getElementById("ctrlTime");
const ctrlTimezoneEl = document.getElementById("ctrlTimezone");
const ctrlLocalEl = document.getElementById("ctrlLocal");
const ctrlUpdatedEl = document.getElementById("ctrlUpdated");
const statusPumpEl = document.getElementById("statusPump");
const statusPumpDurationEl = document.getElementById("statusPumpDuration");
const statusPumpModeEl = document.getElementById("statusPumpMode");
const statusPumpCooldownEl = document.getElementById("statusPumpCooldown");
const statusMoistureEl = document.getElementById("statusMoisture");
const statusLightEl = document.getElementById("statusLight");
const statusLightEffectEl = document.getElementById("statusLightEffect");
const statusLightScheduleEl = document.getElementById("statusLightSchedule");
const statusDeviceSsidEl = document.getElementById("statusDeviceSsid");
const statusDeviceIpEl = document.getElementById("statusDeviceIp");
const pumpStateBadgeEl = document.getElementById("pumpStateBadge");
const pumpFanEl = document.getElementById("pumpFan");
const pumpToggleBtnEl = document.getElementById("pumpToggleBtn");
const pumpDurationInputEl = document.getElementById("pumpDurationInput");
const pumpDurationSaveBtnEl = document.getElementById("pumpDurationSaveBtn");
const lightEnabledToggleEl = document.getElementById("lightEnabledToggle");
const lightEnabledTextEl = document.getElementById("lightEnabledText");
const lightREl = document.getElementById("lightR");
const lightGEl = document.getElementById("lightG");
const lightBEl = document.getElementById("lightB");
const lightPreviewEl = document.getElementById("lightPreview");
const lightValueTextEl = document.getElementById("lightValueText");
const lightApplyBtnEl = document.getElementById("lightApplyBtn");
const lightEffectSelectEl = document.getElementById("lightEffectSelect");
const lightEffectSpeedInputEl = document.getElementById("lightEffectSpeedInput");
const lightEffectApplyBtnEl = document.getElementById("lightEffectApplyBtn");
const manualPumpControlToggleEl = document.getElementById("manualPumpControlToggle");
const manualPumpControlTextEl = document.getElementById("manualPumpControlText");
const autoPumpEnabledToggleEl = document.getElementById("autoPumpEnabledToggle");
const autoPumpEnabledTextEl = document.getElementById("autoPumpEnabledText");
const moistureThresholdInputEl = document.getElementById("moistureThresholdInput");
const autoPumpDurationInputEl = document.getElementById("autoPumpDurationInput");
const autoPumpCooldownInputEl = document.getElementById("autoPumpCooldownInput");
const gardenSettingsSaveBtnEl = document.getElementById("gardenSettingsSaveBtn");
const lightScheduleEnabledToggleEl = document.getElementById("lightScheduleEnabledToggle");
const lightScheduleEnabledTextEl = document.getElementById("lightScheduleEnabledText");
const lightOnTimeInputEl = document.getElementById("lightOnTimeInput");
const lightOffTimeInputEl = document.getElementById("lightOffTimeInput");
const lightScheduleSaveBtnEl = document.getElementById("lightScheduleSaveBtn");
const sensorRefreshBtnEl = document.getElementById("sensorRefreshBtn");
const moisturePercentEl = document.getElementById("moisturePercent");
const moistureRawEl = document.getElementById("moistureRaw");
const moistureBarEl = document.getElementById("moistureBar");
const ctrlState = { statusTimer: null };
const clockState = {
    controllerDisplayText: "-",
    controllerTimeZone: null,
    hasSample: false,
    controllerEpochMs: null,
    sampleCapturedAtMs: null
};

// UI text dictionary.
const ui = {
    de: {
        title: "Vertical Garden Panel",
        subtitle: "Direkter Zugriff im lokalen Netzwerk auf deinen Controller.",
        settings: "Settings",
        advanced: "Advanced Options",
        time: "Zeit einstellen",
        ip: "Controller IP-Adresse",
        timezone: "Zeitzone",
        port: "Custom Port",
        language: "Sprache",
        live: "Live",
        liveStatus: "Live Status",
        connect: "Verbindung pruefen",
        refreshStatus: "Settings aktualisieren",
        timeMode: "Zeitmodus",
        manualTime: "Manuelle Zeit",
        liveRateEnabled: "Eigene Live-Rate verwenden",
        liveInterval: "Live-Intervall (ms)",
        now: "Jetzt (automatisch)",
        manual: "Manuell einstellen",
        send: "Zeit an Controller senden",
        ready: "Bereit. Bitte Controller-IP im lokalen Netzwerk eintragen.",
        auto: "Automatische Zeit: -",
        autoLabel: "Automatische Zeit",
        statusLoading: "Lade Status...",
        statusCheck: "Pruefe Verbindung...",
        statusUpdated: "Status aktualisiert.",
        liveOn: "Live an",
        liveOff: "Live aus",
        connected: "Verbunden",
        notChecked: "Nicht geprueft",
        portEnabled: "Custom Port verwenden"
    },
    en: {
        title: "Vertical Garden Panel",
        subtitle: "Direct access on your local network controller.",
        settings: "Settings",
        advanced: "Advanced Options",
        time: "Set Time",
        ip: "Controller IP address",
        timezone: "Time zone",
        port: "Custom Port",
        language: "Language",
        live: "Live",
        liveStatus: "Live status",
        connect: "Check connection",
        refreshStatus: "Update settings",
        timeMode: "Time mode",
        manualTime: "Manual time",
        liveRateEnabled: "Use custom live rate",
        liveInterval: "Live interval (ms)",
        now: "Now (automatic)",
        manual: "Set manually",
        send: "Send time to controller",
        ready: "Ready. Enter the controller IP on the local network.",
        auto: "Automatic time: -",
        autoLabel: "Automatic time",
        statusLoading: "Loading status...",
        statusCheck: "Checking connection...",
        statusUpdated: "Status updated.",
        liveOn: "Live on",
        liveOff: "Live off",
        connected: "Connected",
        notChecked: "Not checked",
        portEnabled: "Use custom port"
    }
};

// Mutable app flags.
const appState = {
    language: "de",
    live: false,
    portEnabled: false,
    advancedOpen: false,
    liveRateEnabled: false,
    liveInterval: 500,
    lightDraftActive: false
};

const fallbackTimezones = [
    DEFAULT_TIMEZONE,
    "UTC",
    "Europe/Zurich",
    "Europe/Vienna"
];

// Resets projected controller clock values.
function resetClockState() {
    clockState.controllerDisplayText = "-";
    clockState.controllerTimeZone = null;
    clockState.hasSample = false;
    clockState.controllerEpochMs = null;
    clockState.sampleCapturedAtMs = null;
}

function parseControllerIsoToEpochMs(value) {
    if (typeof value !== "string") return null;

    const match = value.trim().match(/^(\d{4})-(\d{2})-(\d{2})[T\s](\d{2}):(\d{2}):(\d{2})$/);
    if (!match) return null;

    const year = Number(match[1]);
    const month = Number(match[2]);
    const day = Number(match[3]);
    const hour = Number(match[4]);
    const minute = Number(match[5]);
    const second = Number(match[6]);

    if (!Number.isFinite(year) || !Number.isFinite(month) || !Number.isFinite(day)
        || !Number.isFinite(hour) || !Number.isFinite(minute) || !Number.isFinite(second)) {
        return null;
    }

    return Date.UTC(year, month - 1, day, hour, minute, second);
}

function formatEpochMsAsControllerText(epochMs) {
    const d = new Date(epochMs);
    const year = d.getUTCFullYear();
    const month = String(d.getUTCMonth() + 1).padStart(2, "0");
    const day = String(d.getUTCDate()).padStart(2, "0");
    const hour = String(d.getUTCHours()).padStart(2, "0");
    const minute = String(d.getUTCMinutes()).padStart(2, "0");
    const second = String(d.getUTCSeconds()).padStart(2, "0");
    return `${year}-${month}-${day} ${hour}:${minute}:${second}`;
}

// Creates AbortController plus timeout cleanup handle.
function createAbortTimeout(ms) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), ms);
    return { controller, timeout };
}

// Theme handling.
function applyTheme(theme) {
    document.documentElement.setAttribute("data-theme", theme);
    themeToggleEl.textContent = theme === "dark" ? "Light Mode" : "Dark Mode";
    localStorage.setItem(storageKeys.theme, theme);
}

function initTheme() {
    const saved = localStorage.getItem(storageKeys.theme);
    if (saved === "dark" || saved === "light") {
        applyTheme(saved);
        return;
    }
    const prefersDark = window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches;
    applyTheme(prefersDark ? "dark" : "light");
}

// Shared status helpers.
function setStatus(text, type) {
    globalStatusEl.textContent = text;
    globalStatusEl.className = "global-status";
    if (type) globalStatusEl.classList.add(type);
}

function setConnectionState(text, type) {
    connectionTextEl.textContent = text;
    connectionDotEl.className = "dot";
    if (type) {
        connectionDotEl.classList.add(type);
    }
}

// Validation and URL helpers.
function isValidIPv4(ip) {
    const parts = ip.trim().split(".");
    if (parts.length !== 4) return false;
    for (const part of parts) {
        if (!/^\d+$/.test(part)) return false;
        const n = Number(part);
        if (n < 0 || n > 255) return false;
    }
    return true;
}

function getPort() {
    if (!appState.portEnabled) {
        return 80;
    }
    const port = Number(portInputEl.value || 80);
    if (!Number.isInteger(port) || port < 1 || port > 65535) {
        return 80;
    }
    return port;
}

function buildBaseUrl() {
    const deviceIp = ipInputEl.value.trim();
    const port = getPort();
    return `http://${deviceIp}:${port}`;
}

// Live mode helpers.
function updateLiveLabel() {
    const text = appState.live ? ui[appState.language].liveOn : ui[appState.language].liveOff;
    liveStatusToggleTextEl.textContent = text;
    liveStatusTitleEl.textContent = ui[appState.language].liveStatus;
}

function getLiveInterval() {
    if (!appState.liveRateEnabled) {
        return 500;
    }
    const value = Number(liveIntervalInputEl.value || 500);
    if (!Number.isFinite(value) || value < 250) {
        return 500;
    }
    return value;
}

function updateLiveRateState() {
    appState.liveRateEnabled = liveRateEnabledEl.checked;
    liveIntervalInputEl.disabled = !appState.liveRateEnabled;
    if (appState.liveRateEnabled) {
        appState.liveInterval = getLiveInterval();
    }
}

// Language and timezone rendering.
function applyLanguage(lang) {
    appState.language = lang;
    languageToggleEl.textContent = lang === "de" ? "EN" : "DE";

    const text = ui[lang];
    titleTextEl.textContent = text.title;
    subtitleTextEl.textContent = text.subtitle;
    settingsTitleEl.textContent = text.settings;
    advancedTitleEl.textContent = text.advanced;
    timeTitleEl.textContent = text.time;
    ipLabelEl.textContent = text.ip;
    timezoneLabelEl.textContent = text.timezone;
    portLabelEl.textContent = text.port;
    liveStatusTitleEl.textContent = text.liveStatus;
    connectBtnEl.textContent = text.connect;
    refreshControllerBtnEl.textContent = text.refreshStatus;
    timeModeLabelEl.textContent = text.timeMode;
    manualTimeLabelEl.textContent = text.manualTime;
    liveRateEnabledLabelEl.textContent = text.liveRateEnabled;
    liveIntervalLabelEl.textContent = text.liveInterval;
    syncBtnEl.textContent = text.send;
    timeModeEl.options[0].textContent = text.now;
    timeModeEl.options[1].textContent = text.manual;
    if (!manualTimeEl.disabled && !manualTimeEl.value) {
        manualTimeEl.value = toLocalDatetimeInputValue(new Date());
    }
    setStatus(text.ready, "muted");
    if (connectionTextEl.textContent === "Nicht geprueft" || connectionTextEl.textContent === "Not checked") {
        setConnectionState(text.notChecked, "");
    }
    updateLiveLabel();
    updateAutoNowPreview();
    localStorage.setItem(storageKeys.language, lang);
}

function loadTimezones() {
    const currentTz = Intl.DateTimeFormat().resolvedOptions().timeZone || DEFAULT_TIMEZONE;
    let zones = fallbackTimezones;

    if (typeof Intl.supportedValuesOf === "function") {
        zones = Intl.supportedValuesOf("timeZone");
    }

    timezoneInputEl.innerHTML = zones
        .map((tz) => `<option value="${tz}">${tz}</option>`)
        .join("");

    timezoneInputEl.value = zones.includes(currentTz) ? currentTz : DEFAULT_TIMEZONE;
}

// Time conversion helpers.
function toLocalDatetimeInputValue(date) {
    const d = new Date(date.getTime() - date.getTimezoneOffset() * 60000);
    return d.toISOString().slice(0, 19);
}

function formatLocalDateTimeForPayload(date, timeZone) {
    const dtf = new Intl.DateTimeFormat("sv-SE", {
        timeZone,
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        hour12: false
    });
    return dtf.format(date).replace(" ", "T");
}

// Clock rendering.
function updateAutoNowPreview() {
    const timezone = timezoneInputEl.value || DEFAULT_TIMEZONE;
    const text = new Date().toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
    autoNowTextEl.textContent = `${ui[appState.language].autoLabel}: ${text} (${timezone})`;
}

function renderLiveClocks() {
    const timezone = timezoneInputEl.value || DEFAULT_TIMEZONE;
    const now = Date.now();

    autoNowTextEl.textContent = `${ui[appState.language].autoLabel}: ${new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone })} (${timezone})`;

    if (!clockState.hasSample) {
        ctrlTimeEl.textContent = "-";
        ctrlLocalEl.textContent = new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
        return;
    }

    if (Number.isFinite(clockState.controllerEpochMs) && Number.isFinite(clockState.sampleCapturedAtMs)) {
        const elapsed = now - clockState.sampleCapturedAtMs;
        const projectedEpochMs = clockState.controllerEpochMs + Math.max(0, elapsed);
        ctrlTimeEl.textContent = formatEpochMsAsControllerText(projectedEpochMs);
    } else {
        ctrlTimeEl.textContent = clockState.controllerDisplayText;
    }
    ctrlLocalEl.textContent = new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
}

function getLightValues() {
    return {
        r: Number(lightREl.value || 0),
        g: Number(lightGEl.value || 0),
        b: Number(lightBEl.value || 0)
    };
}

function formatDurationMs(ms) {
    const n = Number(ms);
    if (!Number.isFinite(n) || n < 0) return "-";
    const totalSec = Math.floor(n / 1000);
    const m = Math.floor(totalSec / 60);
    const s = totalSec % 60;
    return `${m}m ${String(s).padStart(2, "0")}s`;
}

function setIfNotFocused(inputEl, value) {
    if (!inputEl) return;
    if (document.activeElement === inputEl) return;
    inputEl.value = value;
}

function minutesToTimeInputValue(totalMinutes) {
    const n = Number(totalMinutes);
    if (!Number.isFinite(n)) return "00:00";
    const wrapped = ((Math.floor(n) % 1440) + 1440) % 1440;
    const h = String(Math.floor(wrapped / 60)).padStart(2, "0");
    const m = String(wrapped % 60).padStart(2, "0");
    return `${h}:${m}`;
}

function timeInputValueToMinutes(value, fallbackMinutes) {
    if (!value || !/^\d{2}:\d{2}$/.test(value)) {
        return fallbackMinutes;
    }
    const h = Number(value.slice(0, 2));
    const m = Number(value.slice(3, 5));
    if (!Number.isInteger(h) || !Number.isInteger(m) || h < 0 || h > 23 || m < 0 || m > 59) {
        return fallbackMinutes;
    }
    return h * 60 + m;
}

function updatePumpModeLabels() {
    const manualOn = manualPumpControlToggleEl.checked;
    const autoOn = !manualOn;
    autoPumpEnabledToggleEl.checked = autoOn;
    autoPumpEnabledToggleEl.disabled = true;
    manualPumpControlTextEl.textContent = manualOn ? "Manuelle Steuerung aktiv" : "Manuelle Steuerung aus";
    autoPumpEnabledTextEl.textContent = autoOn ? "Automatik an" : "Automatik aus";
    pumpToggleBtnEl.disabled = !manualOn;
}

function updateLightScheduleLabel() {
    lightScheduleEnabledTextEl.textContent = lightScheduleEnabledToggleEl.checked ? "Licht-Zeitplan an" : "Licht-Zeitplan aus";
}

function updateLightPreview() {
    const { r, g, b } = getLightValues();
    lightPreviewEl.style.background = `rgb(${r}, ${g}, ${b})`;
    lightValueTextEl.textContent = `R${r} G${g} B${b}`;
}

function renderGardenData(data) {
    const pumpOn = Boolean(data.pumpOn);
    pumpStateBadgeEl.textContent = pumpOn ? "An" : "Aus";
    pumpStateBadgeEl.className = `state-badge ${pumpOn ? "on" : "off"}`;
    pumpFanEl.classList.toggle("on", pumpOn);

    if (Number.isFinite(Number(data.pumpDurationMs))) {
        setIfNotFocused(pumpDurationInputEl, String(data.pumpDurationMs));
    }
    if (typeof data.manualPumpControlEnabled === "boolean") {
        manualPumpControlToggleEl.checked = data.manualPumpControlEnabled;
    }
    if (typeof data.autoPumpEnabled === "boolean") {
        autoPumpEnabledToggleEl.checked = data.autoPumpEnabled;
    }
    if (Number.isFinite(Number(data.moistureThresholdPercent))) {
        setIfNotFocused(moistureThresholdInputEl, String(data.moistureThresholdPercent));
    }
    if (Number.isFinite(Number(data.autoPumpDurationMs))) {
        setIfNotFocused(autoPumpDurationInputEl, String(data.autoPumpDurationMs));
    }
    if (Number.isFinite(Number(data.autoPumpCooldownMs))) {
        setIfNotFocused(autoPumpCooldownInputEl, String(data.autoPumpCooldownMs));
    }
    updatePumpModeLabels();

    const ledOn = Boolean(data.ledStripOn);
    if (!appState.lightDraftActive) {
        lightEnabledToggleEl.checked = ledOn;
        if (Number.isFinite(Number(data.ledStripR))) setIfNotFocused(lightREl, String(data.ledStripR));
        if (Number.isFinite(Number(data.ledStripG))) setIfNotFocused(lightGEl, String(data.ledStripG));
        if (Number.isFinite(Number(data.ledStripB))) setIfNotFocused(lightBEl, String(data.ledStripB));
        if (typeof data.ledEffect === "string") {
            lightEffectSelectEl.value = data.ledEffect;
        }
        if (Number.isFinite(Number(data.ledEffectSpeedMs))) {
            setIfNotFocused(lightEffectSpeedInputEl, String(data.ledEffectSpeedMs));
        }
        if (typeof data.lightScheduleEnabled === "boolean") {
            lightScheduleEnabledToggleEl.checked = data.lightScheduleEnabled;
        }
        if (Number.isFinite(Number(data.lightOnMinute))) {
            setIfNotFocused(lightOnTimeInputEl, minutesToTimeInputValue(data.lightOnMinute));
        }
        if (Number.isFinite(Number(data.lightOffMinute))) {
            setIfNotFocused(lightOffTimeInputEl, minutesToTimeInputValue(data.lightOffMinute));
        }
    }
    lightEnabledTextEl.textContent = lightEnabledToggleEl.checked ? "An" : "Aus";
    updateLightScheduleLabel();
    updateLightPreview();

    const percent = Number.isFinite(Number(data.moisturePercent)) ? Number(data.moisturePercent) : 0;
    const raw = Number.isFinite(Number(data.moistureRaw)) ? Number(data.moistureRaw) : 0;
    moisturePercentEl.textContent = `${percent}%`;
    moistureRawEl.textContent = String(raw);
    moistureBarEl.style.width = `${Math.max(0, Math.min(100, percent))}%`;

    const remainingMs = Number.isFinite(Number(data.pumpRemainingMs)) ? Number(data.pumpRemainingMs) : 0;
    const pumpDurationMs = Number.isFinite(Number(data.pumpDurationMs)) ? Number(data.pumpDurationMs) : 0;
    statusPumpEl.textContent = pumpOn ? "An" : "Aus";
    statusPumpDurationEl.textContent = `${formatDurationMs(pumpDurationMs)} / ${formatDurationMs(remainingMs)}`;

    const manualEnabled = typeof data.manualPumpControlEnabled === "boolean" ? data.manualPumpControlEnabled : false;
    const autoEnabled = !manualEnabled;
    statusPumpModeEl.textContent = `${manualEnabled ? "Manuell an" : "Manuell aus"} | ${autoEnabled ? "Auto an" : "Auto aus"}`;

    const cooldownMs = Number.isFinite(Number(data.autoPumpCooldownMs)) ? Number(data.autoPumpCooldownMs) : 0;
    statusPumpCooldownEl.textContent = cooldownMs > 0 ? formatDurationMs(cooldownMs) : "-";

    const threshold = Number.isFinite(Number(data.moistureThresholdPercent)) ? Number(data.moistureThresholdPercent) : null;
    statusMoistureEl.textContent = threshold === null ? `${percent}% (${raw})` : `${percent}% (${raw}) | Schwellwert ${threshold}%`;

    statusLightEl.textContent = ledOn ? `An (R${lightREl.value} G${lightGEl.value} B${lightBEl.value})` : "Aus";

    const effect = typeof data.ledEffect === "string" ? data.ledEffect : "static";
    const effectSpeed = Number.isFinite(Number(data.ledEffectSpeedMs)) ? Number(data.ledEffectSpeedMs) : 0;
    statusLightEffectEl.textContent = `${effect} (${effectSpeed} ms)`;

    const scheduleEnabled = typeof data.lightScheduleEnabled === "boolean" ? data.lightScheduleEnabled : false;
    const onMinute = Number.isFinite(Number(data.lightOnMinute)) ? Number(data.lightOnMinute) : 18 * 60;
    const offMinute = Number.isFinite(Number(data.lightOffMinute)) ? Number(data.lightOffMinute) : 23 * 60;
    statusLightScheduleEl.textContent = scheduleEnabled
        ? `${minutesToTimeInputValue(onMinute)}-${minutesToTimeInputValue(offMinute)}`
        : "Aus";

    const apSsid = typeof data.apSsid === "string" && data.apSsid.length > 0 ? data.apSsid : "-";
    const apIp = typeof data.apIp === "string" && data.apIp.length > 0 ? data.apIp : "-";
    const httpPort = Number.isFinite(Number(data.httpPort)) ? Number(data.httpPort) : Number(getPort());
    statusDeviceSsidEl.textContent = apSsid;
    statusDeviceIpEl.textContent = apIp === "-" ? "-" : `${apIp}:${httpPort}`;
}

function renderGardenSettingsData(data) {
    if (typeof data.manualPumpControlEnabled === "boolean") {
        manualPumpControlToggleEl.checked = data.manualPumpControlEnabled;
    }
    if (Number.isFinite(Number(data.moistureThresholdPercent))) {
        setIfNotFocused(moistureThresholdInputEl, String(data.moistureThresholdPercent));
    }
    if (Number.isFinite(Number(data.autoPumpDurationMs))) {
        setIfNotFocused(autoPumpDurationInputEl, String(data.autoPumpDurationMs));
    }
    if (Number.isFinite(Number(data.autoPumpCooldownMs))) {
        setIfNotFocused(autoPumpCooldownInputEl, String(data.autoPumpCooldownMs));
    }
    if (!appState.lightDraftActive) {
        if (typeof data.lightScheduleEnabled === "boolean") {
            lightScheduleEnabledToggleEl.checked = data.lightScheduleEnabled;
        }
        if (Number.isFinite(Number(data.lightOnMinute))) {
            setIfNotFocused(lightOnTimeInputEl, minutesToTimeInputValue(data.lightOnMinute));
        }
        if (Number.isFinite(Number(data.lightOffMinute))) {
            setIfNotFocused(lightOffTimeInputEl, minutesToTimeInputValue(data.lightOffMinute));
        }
        if (typeof data.ledEffect === "string") {
            lightEffectSelectEl.value = data.ledEffect;
        }
        if (Number.isFinite(Number(data.ledEffectSpeedMs))) {
            setIfNotFocused(lightEffectSpeedInputEl, String(data.ledEffectSpeedMs));
        }
    }

    updatePumpModeLabels();
    updateLightScheduleLabel();
}

async function fetchGardenSettings() {
    const endpoint = `${buildBaseUrl()}/api/garden/settings`;
    const response = await fetch(endpoint, { method: "GET" });
    const text = await response.text();
    let data = {};
    try {
        data = JSON.parse(text || "{}");
    } catch {
        data = {};
    }
    if (!response.ok) {
        throw new Error(data.message || text || `HTTP ${response.status}`);
    }
    return data;
}

async function saveGardenSettings() {
    if (!isValidIPv4(ipInputEl.value.trim())) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }

    gardenSettingsSaveBtnEl.disabled = true;
    try {
        const payload = {
            manualPumpControlEnabled: manualPumpControlToggleEl.checked,
            autoPumpEnabled: !manualPumpControlToggleEl.checked,
            moistureThresholdPercent: Number(moistureThresholdInputEl.value || 35),
            autoPumpDurationMs: Number(autoPumpDurationInputEl.value || 5000),
            autoPumpCooldownMs: Number(autoPumpCooldownInputEl.value || 15000),
            lightScheduleEnabled: lightScheduleEnabledToggleEl.checked,
            lightOnMinute: timeInputValueToMinutes(lightOnTimeInputEl.value, 18 * 60),
            lightOffMinute: timeInputValueToMinutes(lightOffTimeInputEl.value, 23 * 60)
        };

        await postJson("/api/garden/settings", payload);
        await fetchControllerData(false);
        setStatus("Garden-Settings gespeichert.", null);
    } catch (error) {
        setStatus(`Garden-Settings Fehler: ${error.message}`, "error");
    } finally {
        gardenSettingsSaveBtnEl.disabled = false;
    }
}

async function saveLightScheduleOnly() {
    appState.lightDraftActive = true;
    try {
        await saveGardenSettings();
    } finally {
        appState.lightDraftActive = false;
    }
}

async function applyLightEffect() {
    if (!isValidIPv4(ipInputEl.value.trim())) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }

    lightEffectApplyBtnEl.disabled = true;
    appState.lightDraftActive = true;
    try {
        const { r, g, b } = getLightValues();
        const autoOn = (r > 0 || g > 0 || b > 0);
        const targetOn = lightEnabledToggleEl.checked || autoOn;
        lightEnabledToggleEl.checked = targetOn;
        lightEnabledTextEl.textContent = targetOn ? "An" : "Aus";
        const payload = {
            on: targetOn,
            r,
            g,
            b,
            effect: lightEffectSelectEl.value,
            effectSpeedMs: Number(lightEffectSpeedInputEl.value || 1200)
        };
        await postJson("/api/led/effect", payload);
        await fetchControllerData(false);
        setStatus("Licht-Effekt aktualisiert.", null);
    } catch (error) {
        setStatus(`Effekt-Fehler: ${error.message}`, "error");
    } finally {
        appState.lightDraftActive = false;
        lightEffectApplyBtnEl.disabled = false;
    }
}

// Controller communication.
async function updateControllerSettings() {
    const deviceIp = ipInputEl.value.trim();
    if (!isValidIPv4(deviceIp)) {
        setStatus(appState.language === "de" ? "Bitte eine gueltige IPv4-Adresse eintragen." : "Please enter a valid IPv4 address.", "warn");
        return;
    }

    saveFormState();
    refreshControllerBtnEl.disabled = true;
    setStatus(appState.language === "de" ? "Sende Settings an den Controller..." : "Sending settings to controller...", "muted");

    const { controller, timeout } = createAbortTimeout(10000);

    try {
        const endpoint = `${buildBaseUrl()}/api/settings`;
        const response = await fetch(endpoint, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                timezone: timezoneInputEl.value,
                liveRateEnabled: appState.liveRateEnabled,
                liveIntervalMs: getLiveInterval()
            }),
            signal: controller.signal
        });

        const text = await response.text();
        if (!response.ok) {
            throw new Error(text || `HTTP ${response.status}`);
        }

        setConnectionState(ui[appState.language].connected, "ok");
        setStatus(appState.language === "de" ? "Settings aktualisiert." : "Settings updated.", null);
        await fetchControllerData(false);
    } catch (error) {
        if (error.name === "AbortError") {
            setConnectionState("Timeout", "error");
            setStatus(appState.language === "de" ? "Timeout: Controller hat nicht rechtzeitig geantwortet." : "Timeout: controller did not respond in time.", "error");
        } else if (error instanceof TypeError) {
            setConnectionState("Blockiert", "error");
            setStatus(appState.language === "de" ? "Browser blockiert die Anfrage (CORS oder HTTPS->HTTP)." : "Browser blocked the request (CORS or HTTPS->HTTP).", "error");
        } else {
            setConnectionState("Fehler", "error");
            setStatus(`${appState.language === "de" ? "Fehler beim Senden:" : "Error sending:"} ${error.message}`, "error");
        }
    } finally {
        clearTimeout(timeout);
        refreshControllerBtnEl.disabled = false;
    }
}

// Manual time parsing and timezone conversion.
function parseDatetimeLocal(value) {
    const match = value.match(/^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2})(?::(\d{2}))?$/);
    if (!match) return null;
    return {
        year: Number(match[1]),
        month: Number(match[2]),
        day: Number(match[3]),
        hour: Number(match[4]),
        minute: Number(match[5]),
        second: Number(match[6] || "0")
    };
}

function getTimeZoneOffsetMs(date, timeZone) {
    const dtf = new Intl.DateTimeFormat("en-US", {
        timeZone,
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        hour12: false
    });

    const parts = dtf.formatToParts(date);
    const map = {};
    for (const part of parts) {
        if (part.type !== "literal") {
            map[part.type] = part.value;
        }
    }

    const asUtc = Date.UTC(
        Number(map.year),
        Number(map.month) - 1,
        Number(map.day),
        Number(map.hour),
        Number(map.minute),
        Number(map.second)
    );
    return asUtc - date.getTime();
}

function zonedDateTimeToUtc(parts, timeZone) {
    const { year, month, day, hour, minute, second } = parts;
    const wallClockUtc = Date.UTC(year, month - 1, day, hour, minute, second);

    // Two-pass conversion handles DST edges for most practical cases.
    let guess = wallClockUtc - getTimeZoneOffsetMs(new Date(wallClockUtc), timeZone);
    guess = wallClockUtc - getTimeZoneOffsetMs(new Date(guess), timeZone);
    return new Date(guess);
}

function updateManualInputState() {
    const isManual = timeModeEl.value === "manual";
    manualTimeEl.disabled = !isManual;
    if (isManual && !manualTimeEl.value) {
        manualTimeEl.value = toLocalDatetimeInputValue(new Date());
    }
}

// Form persistence.
function saveFormState() {
    localStorage.setItem(storageKeys.ip, ipInputEl.value.trim());
    localStorage.setItem(storageKeys.tz, timezoneInputEl.value);
    localStorage.setItem(storageKeys.mode, timeModeEl.value);
    localStorage.setItem(storageKeys.manual, manualTimeEl.value);
    localStorage.setItem(storageKeys.port, String(getPort()));
    localStorage.setItem(storageKeys.language, appState.language);
    localStorage.setItem(storageKeys.live, String(appState.live));
    localStorage.setItem("vg_port_enabled", String(appState.portEnabled));
    localStorage.setItem("vg_advanced_open", String(appState.advancedOpen));
    localStorage.setItem(storageKeys.liveRateEnabled, String(appState.liveRateEnabled));
    localStorage.setItem(storageKeys.liveInterval, String(getLiveInterval()));
}

function clearSavedState() {
    [
        storageKeys.ip,
        storageKeys.tz,
        storageKeys.mode,
        storageKeys.manual,
        storageKeys.theme,
        storageKeys.language,
        storageKeys.port,
        storageKeys.live,
        storageKeys.liveRateEnabled,
        storageKeys.liveInterval,
        "vg_port_enabled",
        "vg_advanced_open"
    ].forEach((key) => localStorage.removeItem(key));
}

function restoreFormState() {
    const savedIp = localStorage.getItem(storageKeys.ip);
    const savedTz = localStorage.getItem(storageKeys.tz);
    const savedMode = localStorage.getItem(storageKeys.mode);
    const savedManual = localStorage.getItem(storageKeys.manual);
    const savedPort = localStorage.getItem(storageKeys.port);
    const savedLanguage = localStorage.getItem(storageKeys.language);
    const savedLive = localStorage.getItem(storageKeys.live);
    const savedPortEnabled = localStorage.getItem("vg_port_enabled");
    const savedAdvancedOpen = localStorage.getItem("vg_advanced_open");
    const savedLiveRateEnabled = localStorage.getItem(storageKeys.liveRateEnabled);
    const savedLiveInterval = localStorage.getItem(storageKeys.liveInterval);

    if (savedIp) ipInputEl.value = savedIp;
    if (savedTz && Array.from(timezoneInputEl.options).some((opt) => opt.value === savedTz)) {
        timezoneInputEl.value = savedTz;
    }
    if (savedPort) portInputEl.value = savedPort;
    appState.portEnabled = savedPortEnabled === "true" ? true : Boolean(savedPort);
    portEnabledEl.checked = appState.portEnabled;
    portInputEl.disabled = !appState.portEnabled;
    if (savedMode === "now" || savedMode === "manual") {
        timeModeEl.value = savedMode;
    }
    if (savedManual) manualTimeEl.value = savedManual;
    if (savedLanguage === "en" || savedLanguage === "de") {
        appState.language = savedLanguage;
    }
    appState.live = savedLive === "true";
    liveStatusToggleEl.checked = appState.live;
    appState.advancedOpen = savedAdvancedOpen === "true";
    advancedContentEl.hidden = !appState.advancedOpen;
    advancedChevronEl.textContent = appState.advancedOpen ? "▴" : "▾";
    appState.liveRateEnabled = savedLiveRateEnabled === "true";
    liveRateEnabledEl.checked = appState.liveRateEnabled;
    if (savedLiveInterval) {
        liveIntervalInputEl.value = savedLiveInterval;
    }
    updateLiveRateState();
    updateManualInputState();
}

// Controller state mapping and polling.
function renderControllerData(data) {
    const normalized = { ...data };
    if (data && typeof data === "object") {
        if (data.pump && typeof data.pump === "object") {
            normalized.pumpOn = data.pump.on;
            normalized.pumpDurationMs = data.pump.durationMs;
            normalized.pumpRemainingMs = data.pump.remainingMs;
            normalized.autoPumpEnabled = data.pump.autoEnabled;
            normalized.manualPumpControlEnabled = data.pump.manualEnabled;
            normalized.moistureThresholdPercent = data.pump.thresholdPercent;
            normalized.autoPumpDurationMs = data.pump.autoDurationMs;
            normalized.autoPumpCooldownMs = data.pump.autoCooldownMs;
        }
        if (data.moisture && typeof data.moisture === "object") {
            normalized.moistureRaw = data.moisture.raw;
            normalized.moisturePercent = data.moisture.percent;
        }
        if (data.ledStrip && typeof data.ledStrip === "object") {
            normalized.ledStripOn = data.ledStrip.on;
            normalized.ledStripR = data.ledStrip.r;
            normalized.ledStripG = data.ledStrip.g;
            normalized.ledStripB = data.ledStrip.b;
            normalized.ledEffect = data.ledStrip.effect;
            normalized.ledEffectSpeedMs = data.ledStrip.effectSpeedMs;
            normalized.lightScheduleEnabled = data.ledStrip.scheduleEnabled;
            normalized.lightOnMinute = data.ledStrip.scheduleOnMinute;
            normalized.lightOffMinute = data.ledStrip.scheduleOffMinute;
        }
    }

    const timezone = normalized.timezone || normalized.tz || "-";
    const local = normalized.localTime || normalized.localDateTime || normalized.time || normalized.datetime || normalized.isoTime || null;

    let displayTime = "-";
    if (local) {
        // Backend sends local wall-clock time, often without timezone suffix.
        displayTime = String(local).replace("T", " ");
    }

    const parsedEpochMs = parseControllerIsoToEpochMs(String(local || ""));
    if (Number.isFinite(parsedEpochMs)) {
        clockState.controllerEpochMs = parsedEpochMs;
        clockState.sampleCapturedAtMs = Date.now();
    } else {
        clockState.controllerEpochMs = null;
        clockState.sampleCapturedAtMs = null;
    }

    clockState.controllerDisplayText = displayTime;
    clockState.controllerTimeZone = timezone !== "-" ? timezone : null;
    clockState.hasSample = displayTime !== "-";

    ctrlTimezoneEl.textContent = timezone;
    ctrlUpdatedEl.textContent = new Date().toLocaleString(DISPLAY_LOCALE);
    renderGardenData(normalized);
    renderLiveClocks();
}

async function postJson(path, payload) {
    const endpoint = `${buildBaseUrl()}${path}`;
    const response = await fetch(endpoint, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
    });
    const text = await response.text();
    let data = {};
    try {
        data = JSON.parse(text || "{}");
    } catch {
        data = { raw: text };
    }
    if (!response.ok) {
        throw new Error(data.message || text || `HTTP ${response.status}`);
    }
    return data;
}

async function togglePump() {
    if (!isValidIPv4(ipInputEl.value.trim())) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }
    pumpToggleBtnEl.disabled = true;
    try {
        const durationMs = Number(pumpDurationInputEl.value || 5000);
        await postJson("/api/pump", { action: "toggle", durationMs });
        await fetchControllerData(false);
        setStatus("Pumpe aktualisiert.", null);
    } catch (error) {
        setStatus(`Pumpenfehler: ${error.message}`, "error");
    } finally {
        pumpToggleBtnEl.disabled = false;
    }
}

async function savePumpDuration() {
    if (!isValidIPv4(ipInputEl.value.trim())) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }
    pumpDurationSaveBtnEl.disabled = true;
    try {
        const pumpDurationMs = Number(pumpDurationInputEl.value || 5000);
        await postJson("/api/pump-duration", { pumpDurationMs });
        await fetchControllerData(false);
        setStatus("Pumpenlaufzeit gespeichert.", null);
    } catch (error) {
        setStatus(`Speicherfehler: ${error.message}`, "error");
    } finally {
        pumpDurationSaveBtnEl.disabled = false;
    }
}

async function applyLight() {
    if (!isValidIPv4(ipInputEl.value.trim())) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }
    lightApplyBtnEl.disabled = true;
    appState.lightDraftActive = true;
    try {
        const { r, g, b } = getLightValues();
        const autoOn = (r > 0 || g > 0 || b > 0);
        const targetOn = lightEnabledToggleEl.checked || autoOn;
        lightEnabledToggleEl.checked = targetOn;
        lightEnabledTextEl.textContent = targetOn ? "An" : "Aus";
        await postJson("/api/led", { on: targetOn, r, g, b });
        await fetchControllerData(false);
        setStatus("RGB aktualisiert.", null);
    } catch (error) {
        setStatus(`RGB-Fehler: ${error.message}`, "error");
    } finally {
        appState.lightDraftActive = false;
        lightApplyBtnEl.disabled = false;
    }
}

async function refreshSensors() {
    await fetchControllerData(false);
}

function stopPolling() {
    if (ctrlState.statusTimer) {
        clearInterval(ctrlState.statusTimer);
        ctrlState.statusTimer = null;
    }
}

function startPolling() {
    stopPolling();
    ctrlState.statusTimer = setInterval(() => {
        fetchControllerData(false);
    }, getLiveInterval());
}

// Full UI reset.
function resetSettings() {
    const confirmText = appState.language === "de"
        ? "Reset wirklich ausfuehren?"
        : "Reset everything?";
    if (!window.confirm(confirmText)) {
        return;
    }

    stopPolling();
    clearSavedState();

    ipInputEl.value = "";
    timezoneInputEl.value = Intl.DateTimeFormat().resolvedOptions().timeZone || DEFAULT_TIMEZONE;
    timeModeEl.value = "now";
    manualTimeEl.value = "";
    portEnabledEl.checked = false;
    appState.portEnabled = false;
    portInputEl.value = "80";
    portInputEl.disabled = true;
    appState.live = false;
    liveStatusToggleEl.checked = false;
    liveRateEnabledEl.checked = false;
    liveIntervalInputEl.value = "500";
    liveIntervalInputEl.disabled = true;
    appState.liveRateEnabled = false;
    appState.liveInterval = 500;
    appState.advancedOpen = false;
    advancedContentEl.hidden = true;
    advancedChevronEl.textContent = "▾";
    appState.language = "de";
    appState.live = false;

    updateManualInputState();
    applyLanguage("de");
    updateLiveLabel();
    updateAutoNowPreview();
    setConnectionState(ui[appState.language].notChecked, "");
    setStatus(ui[appState.language].ready, "muted");
    ctrlTimeEl.textContent = "-";
    ctrlTimezoneEl.textContent = "-";
    ctrlLocalEl.textContent = "-";
    ctrlUpdatedEl.textContent = "-";
    statusPumpEl.textContent = "-";
    statusPumpDurationEl.textContent = "-";
    statusPumpModeEl.textContent = "-";
    statusPumpCooldownEl.textContent = "-";
    statusMoistureEl.textContent = "-";
    statusLightEl.textContent = "-";
    statusLightEffectEl.textContent = "-";
    statusLightScheduleEl.textContent = "-";
    statusDeviceSsidEl.textContent = "-";
    statusDeviceIpEl.textContent = "-";
    resetClockState();
}

// Read status data from known controller endpoints.
async function fetchControllerData(showFeedback = true) {
    const deviceIp = ipInputEl.value.trim();
    if (!isValidIPv4(deviceIp)) {
        setConnectionState("IP fehlt/ungueltig", "error");
        if (showFeedback) setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return null;
    }

    const { controller, timeout } = createAbortTimeout(6000);

    const baseUrl = buildBaseUrl();
    const endpoints = [
        `${baseUrl}/api/status`,
        `${baseUrl}/api/sensors`,
        `${baseUrl}/api/time`,
        `${baseUrl}/status`
    ];

    try {
        for (const endpoint of endpoints) {
            try {
                const response = await fetch(endpoint, {
                    method: "GET",
                    signal: controller.signal
                });
                if (!response.ok) {
                    continue;
                }

                const text = await response.text();
                let data;
                try {
                    data = JSON.parse(text || "{}");
                } catch {
                    data = { raw: text };
                }

                setConnectionState("Verbunden", "ok");
                renderControllerData(data);
                if (endpoint.endsWith("/api/status")) {
                    try {
                        const settings = await fetchGardenSettings();
                        renderGardenSettingsData(settings);
                    } catch {
                        // ignore settings fetch errors to keep status responsive
                    }
                }
                return data;
            } catch {
                // Naechster Endpoint
            }
        }

        setConnectionState("Keine Verbindung", "error");
        if (showFeedback) setStatus("Controller nicht erreichbar oder kein bekanntes Status-Endpoint gefunden.", "error");
        return null;
    } catch (error) {
        if (error.name === "AbortError") {
            setConnectionState("Timeout", "error");
            if (showFeedback) setStatus("Timeout beim Verbinden mit dem Controller.", "error");
        } else {
            setConnectionState("Fehler", "error");
            if (showFeedback) setStatus("Verbindungsfehler (CORS/HTTPS->HTTP moeglich).", "error");
        }
        return null;
    } finally {
        clearTimeout(timeout);
    }
}

// Sends time payload to controller.
async function syncTime() {
    const deviceIp = ipInputEl.value.trim();
    const timezone = timezoneInputEl.value;
    const mode = timeModeEl.value;

    if (!isValidIPv4(deviceIp)) {
        setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        return;
    }

    let selectedDate = new Date();
    if (mode === "now") {
        selectedDate = new Date(Math.floor(Date.now() / 1000) * 1000);
        updateAutoNowPreview();
    }
    if (mode === "manual") {
        if (!manualTimeEl.value) {
            setStatus("Bitte eine manuelle Zeit auswaehlen.", "warn");
            return;
        }
        const parsed = parseDatetimeLocal(manualTimeEl.value);
        if (!parsed) {
            setStatus("Manuelle Zeit ist ungueltig.", "error");
            return;
        }
        selectedDate = zonedDateTimeToUtc(parsed, timezone);
        if (Number.isNaN(selectedDate.getTime())) {
            setStatus("Zeitzonen-Umrechnung fehlgeschlagen.", "error");
            return;
        }
    }

    const payload = {
        timezone,
        localTime: formatLocalDateTimeForPayload(selectedDate, timezone),
        mode
    };

    saveFormState();
    syncBtnEl.disabled = true;
    setStatus("Sende Anfrage an den Controller...", "muted");

    const { controller, timeout } = createAbortTimeout(10000);

    try {
        const endpoint = `${buildBaseUrl()}/api/time`;
        const response = await fetch(endpoint, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(payload),
            signal: controller.signal
        });

        const text = await response.text();
        if (!response.ok) {
            throw new Error(text || `HTTP ${response.status}`);
        }

        setConnectionState("Verbunden", "ok");
        setStatus(`Uhrzeit erfolgreich gesetzt (${selectedDate.toLocaleString(DISPLAY_LOCALE, { timeZone: timezone })} - ${timezone}).`, null);
        await fetchControllerData();
    } catch (error) {
        if (error.name === "AbortError") {
            setConnectionState("Timeout", "error");
            setStatus("Timeout: Controller hat nicht rechtzeitig geantwortet.", "error");
        } else if (error instanceof TypeError) {
            setConnectionState("Blockiert", "error");
            setStatus("Browser blockiert die Anfrage (CORS oder HTTPS->HTTP). Oeffne die Seite lokal per HTTP oder nutze HTTPS auf dem Controller.", "error");
        } else {
            setConnectionState("Fehler", "error");
            setStatus(`Fehler beim Senden: ${error.message}`, "error");
        }
    } finally {
        clearTimeout(timeout);
        syncBtnEl.disabled = false;
    }
}

// Initial setup.
initTheme();
loadTimezones();
restoreFormState();
appState.language = appState.language === "en" ? "en" : "de";
applyLanguage(appState.language);
renderLiveClocks();
setInterval(renderLiveClocks, 250);
if (appState.live) {
    startPolling();
}

// Event wiring.
syncBtnEl.addEventListener("click", syncTime);
connectBtnEl.addEventListener("click", async () => {
    setStatus(appState.language === "de" ? "Pruefe Verbindung..." : "Checking connection...", "muted");
    const data = await fetchControllerData();
    if (data) {
        setStatus(appState.language === "de" ? "Controller erreichbar. Statusdaten wurden aktualisiert." : "Controller reachable. Status data updated.", null);
    }
});
refreshControllerBtnEl.addEventListener("click", async () => {
    await updateControllerSettings();
});
themeToggleEl.addEventListener("click", () => {
    const current = document.documentElement.getAttribute("data-theme") || "light";
    applyTheme(current === "dark" ? "light" : "dark");
});
languageToggleEl.addEventListener("click", () => {
    applyLanguage(appState.language === "de" ? "en" : "de");
    saveFormState();
});
advancedToggleEl.addEventListener("click", () => {
    appState.advancedOpen = !appState.advancedOpen;
    advancedContentEl.hidden = !appState.advancedOpen;
    advancedChevronEl.textContent = appState.advancedOpen ? "▴" : "▾";
    saveFormState();
});
portEnabledEl.addEventListener("change", () => {
    appState.portEnabled = portEnabledEl.checked;
    portInputEl.disabled = !appState.portEnabled;
    saveFormState();
});
liveStatusToggleEl.addEventListener("change", () => {
    appState.live = liveStatusToggleEl.checked;
    updateLiveLabel();
    saveFormState();
    if (appState.live) {
        startPolling();
        fetchControllerData(false);
    } else {
        stopPolling();
    }
});
liveRateEnabledEl.addEventListener("change", () => {
    updateLiveRateState();
    saveFormState();
    if (appState.live) {
        startPolling();
    }
});
liveIntervalInputEl.addEventListener("change", () => {
    appState.liveInterval = getLiveInterval();
    saveFormState();
    if (appState.live) {
        startPolling();
    }
});
timeModeEl.addEventListener("change", () => {
    updateManualInputState();
    saveFormState();
});
ipInputEl.addEventListener("change", () => {
    saveFormState();
    setConnectionState("Nicht geprueft", "");
    resetClockState();
    renderLiveClocks();
});
timezoneInputEl.addEventListener("change", () => {
    saveFormState();
    updateAutoNowPreview();
});
portInputEl.addEventListener("change", saveFormState);
manualTimeEl.addEventListener("change", saveFormState);
resetButtonEl.addEventListener("click", resetSettings);
pumpToggleBtnEl.addEventListener("click", togglePump);
pumpDurationSaveBtnEl.addEventListener("click", savePumpDuration);
lightApplyBtnEl.addEventListener("click", applyLight);
lightEffectApplyBtnEl.addEventListener("click", applyLightEffect);
gardenSettingsSaveBtnEl.addEventListener("click", saveGardenSettings);
lightScheduleSaveBtnEl.addEventListener("click", saveLightScheduleOnly);
sensorRefreshBtnEl.addEventListener("click", refreshSensors);
lightEnabledToggleEl.addEventListener("change", () => {
    appState.lightDraftActive = true;
    lightEnabledTextEl.textContent = lightEnabledToggleEl.checked ? "An" : "Aus";
});
manualPumpControlToggleEl.addEventListener("change", async () => {
    updatePumpModeLabels();
    await saveGardenSettings();
});
autoPumpEnabledToggleEl.addEventListener("change", updatePumpModeLabels);
lightScheduleEnabledToggleEl.addEventListener("change", () => {
    appState.lightDraftActive = true;
    updateLightScheduleLabel();
});
lightREl.addEventListener("input", () => {
    appState.lightDraftActive = true;
    updateLightPreview();
});
lightGEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
    updateLightPreview();
});
lightBEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
    updateLightPreview();
});
lightEffectSelectEl.addEventListener("change", () => {
    appState.lightDraftActive = true;
});
lightEffectSpeedInputEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
});
lightOnTimeInputEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
});
lightOffTimeInputEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
});

// Initial render for advanced section and live status
advancedContentEl.hidden = !appState.advancedOpen;
advancedChevronEl.textContent = appState.advancedOpen ? "▴" : "▾";
liveStatusTitleEl.textContent = ui[appState.language].liveStatus;
liveStatusToggleTextEl.textContent = appState.live ? ui[appState.language].liveOn : ui[appState.language].liveOff;
updateLiveRateState();
updateLightPreview();
updatePumpModeLabels();
updateLightScheduleLabel();
