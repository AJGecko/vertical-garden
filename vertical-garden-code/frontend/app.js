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
const DEFAULT_CONTROLLER_IP = "192.168.4.1";
const AUTO_CONNECT_INTERVAL_MS = 8000;
const STATUS_FETCH_TIMEOUT_MS = 4500;
const API_FETCH_TIMEOUT_MS = 8000;

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
const portEnabledLabelEl = document.getElementById("portEnabledLabel");
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
const statusHubTitleEl = document.getElementById("statusHubTitle");
const statusConnectionTitleEl = document.getElementById("statusConnectionTitle");
const statusGroupTimeTitleEl = document.getElementById("statusGroupTimeTitle");
const statusGroupNetworkTitleEl = document.getElementById("statusGroupNetworkTitle");
const statusGroupPumpTitleEl = document.getElementById("statusGroupPumpTitle");
const statusGroupLightTitleEl = document.getElementById("statusGroupLightTitle");
const statusLabelCtrlTimeEl = document.getElementById("statusLabelCtrlTime");
const statusLabelCtrlTimezoneEl = document.getElementById("statusLabelCtrlTimezone");
const statusLabelLocalTimeEl = document.getElementById("statusLabelLocalTime");
const statusLabelUpdatedEl = document.getElementById("statusLabelUpdated");
const statusLabelDeviceSsidEl = document.getElementById("statusLabelDeviceSsid");
const statusLabelDeviceIpEl = document.getElementById("statusLabelDeviceIp");
const statusLabelPumpEl = document.getElementById("statusLabelPump");
const statusLabelPumpDurationEl = document.getElementById("statusLabelPumpDuration");
const statusLabelPumpModeEl = document.getElementById("statusLabelPumpMode");
const statusLabelPumpCooldownEl = document.getElementById("statusLabelPumpCooldown");
const statusLabelMoistureEl = document.getElementById("statusLabelMoisture");
const statusLabelLightSwatchEl = document.getElementById("statusLabelLightSwatch");
const statusLabelLightEl = document.getElementById("statusLabelLight");
const statusLabelLightEffectEl = document.getElementById("statusLabelLightEffect");
const statusLabelLightScheduleEl = document.getElementById("statusLabelLightSchedule");
const ipLabelEl = document.getElementById("ipLabel");
const timezoneLabelEl = document.getElementById("timezoneLabel");
const portLabelEl = document.getElementById("portLabel");
const timeModeLabelEl = document.getElementById("timeModeLabel");
const manualTimeLabelEl = document.getElementById("manualTimeLabel");
const pumpSensorSectionTitleEl = document.getElementById("pumpSensorSectionTitle");
const lightSectionTitleEl = document.getElementById("lightSectionTitle");

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
const statusMoistureFillEl = document.getElementById("statusMoistureFill");
const statusLightSwatchEl = document.getElementById("statusLightSwatch");
const statusDeviceSsidEl = document.getElementById("statusDeviceSsid");
const statusDeviceIpEl = document.getElementById("statusDeviceIp");
const pumpTitleEl = document.getElementById("pumpTitle");
const sensorTitleEl = document.getElementById("sensorTitle");
const pumpAutoTitleEl = document.getElementById("pumpAutoTitle");
const lightTitleEl = document.getElementById("lightTitle");
const pumpStateBadgeEl = document.getElementById("pumpStateBadge");
const pumpFanEl = document.getElementById("pumpFan");
const pumpToggleBtnEl = document.getElementById("pumpToggleBtn");
const pumpDurationInputEl = document.getElementById("pumpDurationInput");
const pumpDurationSaveBtnEl = document.getElementById("pumpDurationSaveBtn");
const moisturePercentLabelEl = document.getElementById("moisturePercentLabel");
const moistureRawLabelEl = document.getElementById("moistureRawLabel");
const lightREl = document.getElementById("lightR");
const lightGEl = document.getElementById("lightG");
const lightBEl = document.getElementById("lightB");
const lightPreviewEl = document.getElementById("lightPreview");
const lightValueTextEl = document.getElementById("lightValueText");
const lightApplyBtnEl = document.getElementById("lightApplyBtn");
const lightBrightnessEl = document.getElementById("lightBrightness");
const lightBrightnessTextEl = document.getElementById("lightBrightnessText");
const rgbModeToggleEl = document.getElementById("rgbModeToggle");
const rgbModeTextEl = document.getElementById("rgbModeText");
const rgbMenuEl = document.getElementById("rgbMenu");
const rgbControlsEl = document.getElementById("rgbControls");
const lightEffectSelectEl = document.getElementById("lightEffectSelect");
const lightEffectSpeedInputEl = document.getElementById("lightEffectSpeedInput");
const lightEffectSpeedContainerEl = document.getElementById("lightEffectSpeedContainer");
const manualPumpControlToggleEl = document.getElementById("manualPumpControlToggle");
const manualPumpControlTextEl = document.getElementById("manualPumpControlText");
const autoPumpEnabledToggleEl = document.getElementById("autoPumpEnabledToggle");
const autoPumpEnabledTextEl = document.getElementById("autoPumpEnabledText");
const moistureThresholdLabelEl = document.getElementById("moistureThresholdLabel");
const autoPumpDurationLabelEl = document.getElementById("autoPumpDurationLabel");
const autoPumpCooldownLabelEl = document.getElementById("autoPumpCooldownLabel");
const moistureThresholdInputEl = document.getElementById("moistureThresholdInput");
const autoPumpDurationInputEl = document.getElementById("autoPumpDurationInput");
const autoPumpCooldownInputEl = document.getElementById("autoPumpCooldownInput");
const gardenSettingsSaveBtnEl = document.getElementById("gardenSettingsSaveBtn");
const lightApplyHintTextEl = document.getElementById("lightApplyHintText");
const lightBrightnessLabelEl = document.getElementById("lightBrightnessLabel");
const lightZeroHintTextEl = document.getElementById("lightZeroHintText");
const lightEffectLabelEl = document.getElementById("lightEffectLabel");
const lightEffectSpeedLabelEl = document.getElementById("lightEffectSpeedLabel");
const lightOnTimeLabelEl = document.getElementById("lightOnTimeLabel");
const lightOffTimeLabelEl = document.getElementById("lightOffTimeLabel");
const lightScheduleHintTextEl = document.getElementById("lightScheduleHintText");
const lightScheduleEnabledToggleEl = document.getElementById("lightScheduleEnabledToggle");
const lightScheduleEnabledTextEl = document.getElementById("lightScheduleEnabledText");
const lightOnTimeInputEl = document.getElementById("lightOnTimeInput");
const lightOffTimeInputEl = document.getElementById("lightOffTimeInput");
const lightScheduleSaveBtnEl = document.getElementById("lightScheduleSaveBtn");
const sensorRefreshBtnEl = document.getElementById("sensorRefreshBtn");
const moisturePercentEl = document.getElementById("moisturePercent");
const moistureRawEl = document.getElementById("moistureRaw");
const moistureBarEl = document.getElementById("moistureBar");
const ctrlState = {
    statusTimer: null,
    pollInFlight: false,
    fetchInFlight: false,
    lastSettingsFetchMs: 0,
    autoConnectTimer: null,
    lastReachableBaseUrl: null
};
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
        subtitle: "Direkter Zugriff im lokalen Netzwerk auf den Controller.",
        settings: "Verbindung & Grundeinstellungen",
        advanced: "Erweiterte Einstellungen",
        time: "Uhrzeit synchronisieren",
        ip: "Controller IP-Adresse",
        timezone: "Zeitzone",
        port: "Custom Port",
        language: "Sprache",
        live: "Live",
        liveStatus: "Live Status",
        connect: "Controller verbinden",
        refreshStatus: "Controllerdaten laden",
        timeMode: "Synchronisationsmodus",
        manualTime: "Manueller Zeitpunkt",
        liveRateEnabled: "Eigenes Live-Intervall nutzen",
        liveInterval: "Live-Intervall (ms)",
        now: "Jetzt (automatisch)",
        manual: "Manuell einstellen",
        send: "Uhrzeit senden",
        ready: "Bereit. Bitte die Controller-IP im lokalen Netzwerk eintragen.",
        auto: "Automatische Zeit: -",
        autoLabel: "Automatische Zeit",
        statusLoading: "Lade Status...",
        statusCheck: "Pruefe Verbindung...",
        statusUpdated: "Controllerdaten aktualisiert.",
        liveOn: "Live an",
        liveOff: "Live aus",
        connected: "Verbunden",
        notChecked: "Nicht geprueft",
        portEnabled: "Eigenen Port verwenden",
        reset: "Zuruecksetzen",
        collapseSection: "Menue ein- oder ausklappen"
    },
    en: {
        title: "Vertical Garden Panel",
        subtitle: "Direct local network access to your controller.",
        settings: "Connection & Basics",
        advanced: "Advanced settings",
        time: "Synchronize time",
        ip: "Controller IP address",
        timezone: "Time zone",
        port: "Custom Port",
        language: "Language",
        live: "Live",
        liveStatus: "Live status",
        connect: "Connect controller",
        refreshStatus: "Load controller data",
        timeMode: "Sync mode",
        manualTime: "Manual date/time",
        liveRateEnabled: "Use custom live interval",
        liveInterval: "Live interval (ms)",
        now: "Now (automatic)",
        manual: "Set manually",
        send: "Send time",
        ready: "Ready. Enter the controller IP on the local network.",
        auto: "Automatic time: -",
        autoLabel: "Automatic time",
        statusLoading: "Loading status...",
        statusCheck: "Checking connection...",
        statusUpdated: "Controller data updated.",
        liveOn: "Live on",
        liveOff: "Live off",
        connected: "Connected",
        notChecked: "Not checked",
        portEnabled: "Use custom port",
        reset: "Reset",
        collapseSection: "Collapse or expand section"
    }
};

const uiDetail = {
    de: {
        statusHubTitle: "Status",
        statusConnectionTitle: "Verbindungsstatus",
        statusGroupTimeTitle: "Zeit",
        statusGroupNetworkTitle: "Netzwerk & Controller",
        statusGroupPumpTitle: "Pumpe & Feuchtigkeit",
        statusGroupLightTitle: "Licht",
        statusLabelCtrlTime: "Controller-Uhrzeit",
        statusLabelCtrlTimezone: "Controller-Zeitzone",
        statusLabelLocalTime: "Lokale Zeit",
        statusLabelUpdated: "Letzte Aktualisierung",
        statusLabelDeviceSsid: "Controller (SSID)",
        statusLabelDeviceIp: "Controller (IP:Port)",
        statusLabelPump: "Pumpe",
        statusLabelPumpDuration: "Laufzeit / Restzeit",
        statusLabelPumpMode: "Pumpenmodus",
        statusLabelPumpCooldown: "Automatik-Cooldown",
        statusLabelMoisture: "Feuchtigkeit",
        statusLabelLightSwatch: "Aktive Farbe",
        statusLabelLight: "Licht",
        statusLabelLightEffect: "Lichteffekt",
        statusLabelLightSchedule: "Zeitplan",
        pumpSensorSectionTitle: "Pumpe & Sensoren",
        lightSectionTitle: "Licht",
        pumpTitle: "Pumpe",
        pumpToggleBtn: "Pumpe starten/stoppen",
        pumpDurationSaveBtn: "Pumpenlaufzeit speichern",
        sensorTitle: "Bodenfeuchte-Sensor",
        sensorRefreshBtn: "Aktualisieren",
        moisturePercentLabel: "Feuchtigkeit",
        moistureRawLabel: "Sensorwert (Raw)",
        pumpAutoTitle: "Pumpen-Automatik",
        moistureThresholdLabel: "Feuchtigkeits-Schwellwert (%)",
        autoPumpDurationLabel: "Auto Laufzeit (ms)",
        autoPumpCooldownLabel: "Auto Cooldown (ms)",
        gardenSettingsSaveBtn: "Automatik-Einstellungen speichern",
        lightTitle: "Lichtsteuerung",
        lightApplyHintText: "Farbe, Helligkeit und Effekt werden gemeinsam mit \"Licht speichern & anwenden\" uebernommen.",
        lightBrightnessLabel: "Helligkeit",
        lightZeroHintText: "Hinweis: 0% Helligkeit entspricht Aus.",
        lightApplyBtn: "Licht speichern & anwenden",
        lightEffectLabel: "Effekt",
        lightEffectSpeedLabel: "Effekttempo (ms)",
        lightOnTimeLabel: "Licht an (Uhrzeit)",
        lightOffTimeLabel: "Licht aus (Uhrzeit)",
        lightScheduleSaveBtn: "Zeitplan speichern",
        lightScheduleHintText: "Der Zeitplan nutzt die zuletzt oben mit \"Licht speichern & anwenden\" gesicherte Farbe.",
        lightScheduleOn: "Zeitplan aktiv",
        lightScheduleOff: "Zeitplan aus",
        rgbModeOn: "RGB-Modus aktiv",
        rgbModeOff: "RGB-Modus aktivieren",
        manualControlOn: "Manueller Modus aktiv",
        manualControlOff: "Manueller Modus aus",
        automationOn: "Automatik aktiv",
        automationOff: "Automatik aus",
        on: "An",
        off: "Aus",
        thresholdWord: "Schwellwert",
        darkMode: "Dunkler Modus",
        lightMode: "Heller Modus",
        effects: {
            static: "Statisch (Standard)",
            blink: "Blinken",
            breathe: "Atmen",
            waves: "Wellen",
            rainbow: "Regenbogen"
        }
    },
    en: {
        statusHubTitle: "Status",
        statusConnectionTitle: "Connection status",
        statusGroupTimeTitle: "Time",
        statusGroupNetworkTitle: "Network & Controller",
        statusGroupPumpTitle: "Pump & Moisture",
        statusGroupLightTitle: "Light",
        statusLabelCtrlTime: "Controller time",
        statusLabelCtrlTimezone: "Controller time zone",
        statusLabelLocalTime: "Local time",
        statusLabelUpdated: "Last update",
        statusLabelDeviceSsid: "Controller (SSID)",
        statusLabelDeviceIp: "Controller (IP:Port)",
        statusLabelPump: "Pump",
        statusLabelPumpDuration: "Runtime / remaining",
        statusLabelPumpMode: "Pump mode",
        statusLabelPumpCooldown: "Automation cooldown",
        statusLabelMoisture: "Moisture",
        statusLabelLightSwatch: "Active color",
        statusLabelLight: "Light",
        statusLabelLightEffect: "Lighting effect",
        statusLabelLightSchedule: "Schedule",
        pumpSensorSectionTitle: "Pump & Sensors",
        lightSectionTitle: "Light",
        pumpTitle: "Pump",
        pumpToggleBtn: "Start/stop pump",
        pumpDurationSaveBtn: "Save pump runtime",
        sensorTitle: "Soil moisture sensor",
        sensorRefreshBtn: "Refresh",
        moisturePercentLabel: "Moisture",
        moistureRawLabel: "Sensor value (raw)",
        pumpAutoTitle: "Pump automation",
        moistureThresholdLabel: "Moisture threshold (%)",
        autoPumpDurationLabel: "Auto runtime (ms)",
        autoPumpCooldownLabel: "Auto cooldown (ms)",
        gardenSettingsSaveBtn: "Save automation settings",
        lightTitle: "Light control",
        lightApplyHintText: "Color, brightness, and effect are applied together with \"Save & Apply Light\".",
        lightBrightnessLabel: "Brightness",
        lightZeroHintText: "Note: 0% brightness means off.",
        lightApplyBtn: "Save & Apply Light",
        lightEffectLabel: "Effect",
        lightEffectSpeedLabel: "Effect speed (ms)",
        lightOnTimeLabel: "Light on (time)",
        lightOffTimeLabel: "Light off (time)",
        lightScheduleSaveBtn: "Save schedule",
        lightScheduleHintText: "The schedule uses the color last saved above with \"Save & Apply Light\".",
        lightScheduleOn: "Schedule on",
        lightScheduleOff: "Schedule off",
        rgbModeOn: "RGB mode on",
        rgbModeOff: "Enable RGB mode",
        manualControlOn: "Manual mode on",
        manualControlOff: "Manual mode off",
        automationOn: "Automation on",
        automationOff: "Automation off",
        on: "On",
        off: "Off",
        thresholdWord: "Threshold",
        darkMode: "Dark mode",
        lightMode: "Light mode",
        effects: {
            static: "Static (Standard)",
            blink: "Blink",
            breathe: "Breathe",
            waves: "Waves",
            rainbow: "Rainbow"
        }
    }
};

// Mutable app flags.
const appState = {
    language: "de",
    live: false,
    portEnabled: false,
    advancedOpen: false,
    liveRateEnabled: false,
    liveInterval: 4000,
    lightDraftActive: false,
    rgbModeEnabled: false,
    lightSliderDragging: false
};

const localLightState = {
    hasValue: false,
    on: false,
    r: 0,
    g: 0,
    b: 0,
    effect: "static",
    effectSpeedMs: 1200,
    scheduleEnabled: null,
    scheduleOnMinute: null,
    scheduleOffMinute: null
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

    const raw = value.trim();
    if (!raw) return null;

    const match = raw.match(/^(\d{4})-(\d{2})-(\d{2})[T\s](\d{2}):(\d{2})(?::(\d{2}))?(?:\.(\d{1,3}))?(Z|[+\-]\d{2}:?\d{2})?$/);
    if (!match) {
        const normalized = raw.includes("T") ? raw : raw.replace(" ", "T");
        const parsed = Date.parse(normalized);
        return Number.isFinite(parsed) ? parsed : null;
    }

    const year = Number(match[1]);
    const month = Number(match[2]);
    const day = Number(match[3]);
    const hour = Number(match[4]);
    const minute = Number(match[5]);
    const second = Number(match[6] || "0");
    const millis = Number((match[7] || "0").padEnd(3, "0"));
    const zone = match[8] || "";

    if (!Number.isFinite(year) || !Number.isFinite(month) || !Number.isFinite(day)
        || !Number.isFinite(hour) || !Number.isFinite(minute) || !Number.isFinite(second) || !Number.isFinite(millis)) {
        return null;
    }

    if (!zone) {
        return Date.UTC(year, month - 1, day, hour, minute, second, millis);
    }

    const zoneNormalized = zone === "Z" ? "Z" : (zone.includes(":") ? zone : `${zone.slice(0, 3)}:${zone.slice(3)}`);
    const iso = `${match[1]}-${match[2]}-${match[3]}T${match[4]}:${match[5]}:${String(second).padStart(2, "0")}.${String(millis).padStart(3, "0")}${zoneNormalized}`;
    const parsed = Date.parse(iso);
    return Number.isFinite(parsed) ? parsed : null;
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
    const detail = currentUiDetail();
    document.documentElement.setAttribute("data-theme", theme);
    themeToggleEl.textContent = theme === "dark" ? detail.lightMode : detail.darkMode;
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

function getCandidateBaseUrls(deviceIp) {
    const ip = deviceIp.trim();
    if (!ip) return [];

    const urls = [];
    const pushUnique = (url) => {
        if (url && !urls.includes(url)) {
            urls.push(url);
        }
    };

    if (appState.portEnabled) {
        pushUnique(`http://${ip}:${getPort()}`);
    }
    pushUnique(`http://${ip}:80`);
    pushUnique(`http://${ip}`);
    return urls;
}

function getPrioritizedBaseUrls(deviceIp) {
    const ip = deviceIp.trim();
    const candidates = getCandidateBaseUrls(ip);
    const remembered = ctrlState.lastReachableBaseUrl;

    if (!remembered) {
        return candidates;
    }

    try {
        const parsed = new URL(remembered);
        if (parsed.hostname !== ip) {
            return candidates;
        }
    } catch {
        return candidates;
    }

    return [remembered, ...candidates.filter((url) => url !== remembered)];
}

function rememberWorkingBaseUrl(baseUrl) {
    ctrlState.lastReachableBaseUrl = baseUrl;

    let parsedPort = 80;
    try {
        const parsed = new URL(baseUrl);
        if (parsed.port) {
            const asNumber = Number(parsed.port);
            if (Number.isInteger(asNumber) && asNumber >= 1 && asNumber <= 65535) {
                parsedPort = asNumber;
            }
        }
    } catch {
        return;
    }

    const shouldEnableCustomPort = parsedPort !== 80;
    let changed = false;

    if (Number(portInputEl.value || 80) !== parsedPort) {
        portInputEl.value = String(parsedPort);
        changed = true;
    }

    if (appState.portEnabled !== shouldEnableCustomPort) {
        appState.portEnabled = shouldEnableCustomPort;
        portEnabledEl.checked = shouldEnableCustomPort;
        portInputEl.disabled = !shouldEnableCustomPort;
        changed = true;
    }

    if (changed) {
        saveFormState();
    }
}

function isObjectRecord(value) {
    return Boolean(value) && typeof value === "object" && !Array.isArray(value);
}

function isLikelyControllerStatusPayload(data) {
    if (!isObjectRecord(data)) return false;
    return (
        "timezone" in data
        || "localTime" in data
        || "moisturePercent" in data
        || "pumpOn" in data
        || "pump" in data
        || "ledStrip" in data
    );
}

function isLikelyControllerAckPayload(data) {
    if (!isObjectRecord(data)) return false;
    return (
        typeof data.ok === "boolean"
        || "pumpOn" in data
        || "localTime" in data
        || "timezone" in data
    );
}

async function fetchJsonFromController(path, options = {}) {
    const {
        method = "GET",
        payload = null,
        timeoutMs = STATUS_FETCH_TIMEOUT_MS,
        validate = null
    } = options;

    const deviceIp = ipInputEl.value.trim();
    const baseUrls = getPrioritizedBaseUrls(deviceIp);
    let lastError = new Error("Controller nicht erreichbar");

    for (const baseUrl of baseUrls) {
        const endpoint = `${baseUrl}${path}`;
        const { controller, timeout } = createAbortTimeout(timeoutMs);

        try {
            const requestInit = {
                method,
                signal: controller.signal,
                cache: "no-store",
                keepalive: false
            };

            if (method !== "GET" && payload !== null) {
                requestInit.headers = { "Content-Type": "application/json" };
                requestInit.body = JSON.stringify(payload);
            }

            const response = await fetch(endpoint, requestInit);
            const text = await response.text();
            let data = {};

            try {
                data = JSON.parse(text || "{}");
            } catch {
                data = { raw: text };
            }

            if (!response.ok) {
                const status = response.status;
                const shouldTryNextBase = status === 404 || status === 405 || status === 501;
                const err = new Error(data.message || text || `HTTP ${status}`);
                err.stopTrying = !shouldTryNextBase;
                if (shouldTryNextBase) {
                    lastError = err;
                    continue;
                }
                throw err;
            }

            if (typeof validate === "function" && !validate(data)) {
                lastError = new Error("Unerwartete Antwort vom Zielhost.");
                continue;
            }

            rememberWorkingBaseUrl(baseUrl);
            return data;
        } catch (error) {
            if (error && error.stopTrying) {
                throw error;
            }
            lastError = error;
        } finally {
            clearTimeout(timeout);
        }
    }

    throw lastError;
}

function buildBaseUrl() {
    const deviceIp = ipInputEl.value.trim();
    const prioritized = getPrioritizedBaseUrls(deviceIp);
    if (prioritized.length > 0) {
        return prioritized[0];
    }
    return `http://${deviceIp}:80`;
}

// Live mode helpers.
function updateLiveLabel() {
    const text = appState.live ? ui[appState.language].liveOn : ui[appState.language].liveOff;
    liveStatusToggleTextEl.textContent = text;
    liveStatusTitleEl.textContent = ui[appState.language].liveStatus;
}

function getLiveInterval() {
    if (!appState.liveRateEnabled) {
        return 4000;
    }
    const value = Number(liveIntervalInputEl.value || 4000);
    if (!Number.isFinite(value) || value < 1500) {
        return 4000;
    }
    return value;
}

function updateLiveRateState() {
    appState.liveRateEnabled = liveRateEnabledEl.checked;
    liveIntervalInputEl.disabled = !appState.liveRateEnabled;
    if (appState.liveRateEnabled) {
        appState.liveInterval = getLiveInterval();
    } else {
        appState.liveInterval = 4000;
    }
}

function setupSectionCollapsibles() {
    const sections = Array.from(document.querySelectorAll(".panel-body > .section"));
    sections.forEach((section, index) => {
        const titleEl = section.querySelector(".section-title");
        if (!titleEl) return;

        const bodyEl = document.createElement("div");
        bodyEl.className = "section-body";
        bodyEl.id = `section-body-${index + 1}`;

        while (titleEl.nextSibling) {
            bodyEl.append(titleEl.nextSibling);
        }
        section.append(bodyEl);

        let actionsEl = titleEl.querySelector(".tools");
        if (!actionsEl) {
            actionsEl = document.createElement("div");
            actionsEl.className = "section-title-actions";
            titleEl.append(actionsEl);
        }

        const toggleBtnEl = document.createElement("button");
        toggleBtnEl.type = "button";
        toggleBtnEl.className = "secondary tiny-button section-collapse-btn";
        toggleBtnEl.setAttribute("aria-controls", bodyEl.id);
        toggleBtnEl.setAttribute("aria-expanded", "false");
        toggleBtnEl.title = ui[appState.language].collapseSection;
        toggleBtnEl.setAttribute("aria-label", ui[appState.language].collapseSection);

        const chevronEl = document.createElement("span");
        chevronEl.className = "section-collapse-chevron";
        chevronEl.setAttribute("aria-hidden", "true");
        toggleBtnEl.append(chevronEl);
        actionsEl.append(toggleBtnEl);

        function setExpanded(expanded) {
            bodyEl.hidden = !expanded;
            section.classList.toggle("section-collapsed", !expanded);
            toggleBtnEl.setAttribute("aria-expanded", String(expanded));
            chevronEl.textContent = expanded ? "▾" : "▸";
        }

        setExpanded(false);
        toggleBtnEl.addEventListener("click", () => {
            const expanded = toggleBtnEl.getAttribute("aria-expanded") === "true";
            setExpanded(!expanded);
        });
    });
}

function currentUiDetail() {
    return uiDetail[appState.language] || uiDetail.de;
}

function effectLabel(effectName) {
    const detail = currentUiDetail();
    const key = String(effectName || "static").toLowerCase();
    return detail.effects[key] || key;
}

function applyEffectOptionLabels() {
    if (!lightEffectSelectEl) return;
    const detail = currentUiDetail();
    Array.from(lightEffectSelectEl.options).forEach((option) => {
        const key = String(option.value || "").toLowerCase();
        option.textContent = detail.effects[key] || option.textContent;
    });
}

// Language and timezone rendering.
function applyLanguage(lang) {
    appState.language = lang;
    languageToggleEl.textContent = lang === "de" ? "EN" : "DE";

    const text = ui[lang];
    const detail = uiDetail[lang] || uiDetail.de;
    titleTextEl.textContent = text.title;
    subtitleTextEl.textContent = text.subtitle;
    settingsTitleEl.textContent = text.settings;
    advancedTitleEl.textContent = text.advanced;
    timeTitleEl.textContent = text.time;
    statusHubTitleEl.textContent = detail.statusHubTitle;
    statusConnectionTitleEl.textContent = detail.statusConnectionTitle;
    statusGroupTimeTitleEl.textContent = detail.statusGroupTimeTitle;
    statusGroupNetworkTitleEl.textContent = detail.statusGroupNetworkTitle;
    statusGroupPumpTitleEl.textContent = detail.statusGroupPumpTitle;
    statusGroupLightTitleEl.textContent = detail.statusGroupLightTitle;
    statusLabelCtrlTimeEl.textContent = detail.statusLabelCtrlTime;
    statusLabelCtrlTimezoneEl.textContent = detail.statusLabelCtrlTimezone;
    statusLabelLocalTimeEl.textContent = detail.statusLabelLocalTime;
    statusLabelUpdatedEl.textContent = detail.statusLabelUpdated;
    statusLabelDeviceSsidEl.textContent = detail.statusLabelDeviceSsid;
    statusLabelDeviceIpEl.textContent = detail.statusLabelDeviceIp;
    statusLabelPumpEl.textContent = detail.statusLabelPump;
    statusLabelPumpDurationEl.textContent = detail.statusLabelPumpDuration;
    statusLabelPumpModeEl.textContent = detail.statusLabelPumpMode;
    statusLabelPumpCooldownEl.textContent = detail.statusLabelPumpCooldown;
    statusLabelMoistureEl.textContent = detail.statusLabelMoisture;
    statusLabelLightSwatchEl.textContent = detail.statusLabelLightSwatch;
    statusLabelLightEl.textContent = detail.statusLabelLight;
    statusLabelLightEffectEl.textContent = detail.statusLabelLightEffect;
    statusLabelLightScheduleEl.textContent = detail.statusLabelLightSchedule;
    ipLabelEl.textContent = text.ip;
    timezoneLabelEl.textContent = text.timezone;
    portLabelEl.textContent = text.port;
    portEnabledLabelEl.textContent = text.portEnabled;
    liveStatusTitleEl.textContent = text.liveStatus;
    pumpSensorSectionTitleEl.textContent = detail.pumpSensorSectionTitle;
    lightSectionTitleEl.textContent = detail.lightSectionTitle;
    pumpTitleEl.textContent = detail.pumpTitle;
    pumpToggleBtnEl.textContent = detail.pumpToggleBtn;
    pumpDurationSaveBtnEl.textContent = detail.pumpDurationSaveBtn;
    sensorTitleEl.textContent = detail.sensorTitle;
    sensorRefreshBtnEl.textContent = detail.sensorRefreshBtn;
    moisturePercentLabelEl.textContent = detail.moisturePercentLabel;
    moistureRawLabelEl.textContent = detail.moistureRawLabel;
    pumpAutoTitleEl.textContent = detail.pumpAutoTitle;
    moistureThresholdLabelEl.textContent = detail.moistureThresholdLabel;
    autoPumpDurationLabelEl.textContent = detail.autoPumpDurationLabel;
    autoPumpCooldownLabelEl.textContent = detail.autoPumpCooldownLabel;
    gardenSettingsSaveBtnEl.textContent = detail.gardenSettingsSaveBtn;
    lightTitleEl.textContent = detail.lightTitle;
    lightApplyHintTextEl.textContent = detail.lightApplyHintText;
    lightBrightnessLabelEl.textContent = detail.lightBrightnessLabel;
    lightZeroHintTextEl.textContent = detail.lightZeroHintText;
    lightApplyBtnEl.textContent = detail.lightApplyBtn;
    lightEffectLabelEl.textContent = detail.lightEffectLabel;
    lightEffectSpeedLabelEl.textContent = detail.lightEffectSpeedLabel;
    lightOnTimeLabelEl.textContent = detail.lightOnTimeLabel;
    lightOffTimeLabelEl.textContent = detail.lightOffTimeLabel;
    lightScheduleSaveBtnEl.textContent = detail.lightScheduleSaveBtn;
    lightScheduleHintTextEl.textContent = detail.lightScheduleHintText;
    connectBtnEl.textContent = text.connect;
    refreshControllerBtnEl.textContent = text.refreshStatus;
    resetButtonEl.textContent = text.reset;
    timeModeLabelEl.textContent = text.timeMode;
    manualTimeLabelEl.textContent = text.manualTime;
    liveRateEnabledLabelEl.textContent = text.liveRateEnabled;
    liveIntervalLabelEl.textContent = text.liveInterval;
    syncBtnEl.textContent = text.send;
    timeModeEl.options[0].textContent = text.now;
    timeModeEl.options[1].textContent = text.manual;
    document.querySelectorAll(".section-collapse-btn").forEach((btn) => {
        btn.title = text.collapseSection;
        btn.setAttribute("aria-label", text.collapseSection);
    });
    updatePumpModeLabels();
    updateLightScheduleLabel();
    pumpStateBadgeEl.textContent = pumpStateBadgeEl.classList.contains("on") ? detail.on : detail.off;
    rgbModeTextEl.textContent = appState.rgbModeEnabled ? detail.rgbModeOn : detail.rgbModeOff;
    applyEffectOptionLabels();
    if (!manualTimeEl.disabled && !manualTimeEl.value) {
        manualTimeEl.value = toLocalDatetimeInputValue(new Date());
    }
    setStatus(text.ready, "muted");
    if (connectionTextEl.textContent === "Nicht geprueft"
        || connectionTextEl.textContent === "Nicht geprüft"
        || connectionTextEl.textContent === "Not checked") {
        setConnectionState(text.notChecked, "");
    }
    applyTheme(document.documentElement.getAttribute("data-theme") || "light");
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

    ctrlTimeEl.textContent = clockState.controllerDisplayText;
    ctrlLocalEl.textContent = new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
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
    const detail = currentUiDetail();
    const manualOn = manualPumpControlToggleEl.checked;
    const autoOn = !manualOn;
    autoPumpEnabledToggleEl.checked = autoOn;
    autoPumpEnabledToggleEl.disabled = true;
    manualPumpControlTextEl.textContent = manualOn ? detail.manualControlOn : detail.manualControlOff;
    autoPumpEnabledTextEl.textContent = autoOn ? detail.automationOn : detail.automationOff;
    pumpToggleBtnEl.disabled = !manualOn;
}

function updateLightScheduleLabel() {
    const detail = currentUiDetail();
    lightScheduleEnabledTextEl.textContent = lightScheduleEnabledToggleEl.checked ? detail.lightScheduleOn : detail.lightScheduleOff;
}

function updateLightPreview() {
    const { r, g, b } = getLightValues();
    const color = `rgb(${r}, ${g}, ${b})`;
    lightPreviewEl.style.background = color;
    lightValueTextEl.textContent = `R${r} G${g} B${b}`;
    const brightness = Number(lightBrightnessEl.value || 100);
    lightBrightnessTextEl.textContent = `${brightness}%`;
    if (lightBrightnessEl) {
        lightBrightnessEl.style.removeProperty("accent-color");
        lightBrightnessEl.style.removeProperty("background");
    }
}

function updateLightControlsState() {
    setRgbMode(rgbModeToggleEl.checked);
    updateEffectSpeedVisibility();
}

function updateEffectSpeedVisibility() {
    if (!lightEffectSelectEl || !lightEffectSpeedContainerEl) return;
    if (lightEffectSelectEl.value === "static") {
        lightEffectSpeedContainerEl.style.display = "none";
    } else {
        lightEffectSpeedContainerEl.style.display = "block";
    }
}

function setRgbMode(enabled) {
    const detail = currentUiDetail();
    appState.rgbModeEnabled = enabled;
    rgbModeTextEl.textContent = enabled ? detail.rgbModeOn : detail.rgbModeOff;
    if (rgbMenuEl) {
        rgbMenuEl.hidden = !enabled;
    }
    if (rgbControlsEl) {
        const controls = Array.from(rgbControlsEl.querySelectorAll("input, select"));
        controls.forEach((input) => {
            input.disabled = !enabled;
        });
    }
    lightEffectSelectEl.disabled = false;
    lightEffectSpeedInputEl.disabled = false;
    updateLightPreview();
}

function getLightValues() {
    const brightness = Number(lightBrightnessEl.value || 100) / 100;
    if (appState.rgbModeEnabled) {
        const baseR = Number(lightREl.value || 0);
        const baseG = Number(lightGEl.value || 0);
        const baseB = Number(lightBEl.value || 0);
        if (baseR === 0 && baseG === 0 && baseB === 0 && brightness > 0) {
            const neutral = Math.round(255 * brightness);
            return { r: neutral, g: neutral, b: neutral };
        }
        return {
            r: Math.round(baseR * brightness),
            g: Math.round(baseG * brightness),
            b: Math.round(baseB * brightness)
        };
    }
    const white = Math.round(255 * brightness);
    return { r: white, g: white, b: white };
}

function mergeLocalLightState(data) {
    if (!data || typeof data !== "object") {
        return;
    }

    let touched = false;

    const nextOn = typeof data.ledStripOn === "boolean"
        ? data.ledStripOn
        : (typeof data.on === "boolean" ? data.on : null);
    if (typeof nextOn === "boolean") {
        localLightState.on = nextOn;
        touched = true;
    }

    const nextR = Number.isFinite(Number(data.ledStripR))
        ? Number(data.ledStripR)
        : (Number.isFinite(Number(data.r)) ? Number(data.r) : null);
    const nextG = Number.isFinite(Number(data.ledStripG))
        ? Number(data.ledStripG)
        : (Number.isFinite(Number(data.g)) ? Number(data.g) : null);
    const nextB = Number.isFinite(Number(data.ledStripB))
        ? Number(data.ledStripB)
        : (Number.isFinite(Number(data.b)) ? Number(data.b) : null);

    if (Number.isFinite(nextR) && Number.isFinite(nextG) && Number.isFinite(nextB)) {
        localLightState.r = Math.max(0, Math.min(255, Math.round(nextR)));
        localLightState.g = Math.max(0, Math.min(255, Math.round(nextG)));
        localLightState.b = Math.max(0, Math.min(255, Math.round(nextB)));
        touched = true;
    }

    const nextEffect = typeof data.ledEffect === "string"
        ? data.ledEffect
        : (typeof data.effect === "string" ? data.effect : null);
    if (nextEffect) {
        localLightState.effect = nextEffect;
        touched = true;
    }

    const nextEffectSpeed = Number.isFinite(Number(data.ledEffectSpeedMs))
        ? Number(data.ledEffectSpeedMs)
        : (Number.isFinite(Number(data.effectSpeedMs)) ? Number(data.effectSpeedMs) : null);
    if (Number.isFinite(nextEffectSpeed)) {
        localLightState.effectSpeedMs = Math.max(120, Math.min(10000, Math.round(nextEffectSpeed)));
        touched = true;
    }

    if (typeof data.lightScheduleEnabled === "boolean") {
        localLightState.scheduleEnabled = data.lightScheduleEnabled;
        touched = true;
    }
    if (Number.isFinite(Number(data.lightOnMinute))) {
        localLightState.scheduleOnMinute = Number(data.lightOnMinute);
        touched = true;
    }
    if (Number.isFinite(Number(data.lightOffMinute))) {
        localLightState.scheduleOffMinute = Number(data.lightOffMinute);
        touched = true;
    }

    if (touched) {
        localLightState.hasValue = true;
    }
}

function renderLightStatusFromLocalState() {
    if (!localLightState.hasValue) {
        return;
    }

    const detail = currentUiDetail();
    statusLightEl.textContent = localLightState.on
        ? `${detail.on} (R${localLightState.r} G${localLightState.g} B${localLightState.b})`
        : detail.off;

    if (statusLightSwatchEl) {
        statusLightSwatchEl.style.background = localLightState.on
            ? `rgb(${localLightState.r}, ${localLightState.g}, ${localLightState.b})`
            : "color-mix(in srgb, var(--soft), var(--card) 50%)";
    }

    statusLightEffectEl.textContent = `${effectLabel(localLightState.effect)} (${localLightState.effectSpeedMs} ms)`;

    if (typeof localLightState.scheduleEnabled === "boolean") {
        statusLightScheduleEl.textContent = localLightState.scheduleEnabled
            ? `${minutesToTimeInputValue(localLightState.scheduleOnMinute ?? 18 * 60)}-${minutesToTimeInputValue(localLightState.scheduleOffMinute ?? 23 * 60)}`
            : detail.off;
    }
}

function renderGardenData(data) {
    const detail = currentUiDetail();
    const pumpOn = Boolean(data.pumpOn);
    pumpStateBadgeEl.textContent = pumpOn ? detail.on : detail.off;
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
    if (!appState.lightDraftActive && !appState.lightSliderDragging) {
        const r = Number(data.ledStripR);
        const g = Number(data.ledStripG);
        const b = Number(data.ledStripB);
        if (Number.isFinite(r) && Number.isFinite(g) && Number.isFinite(b)) {
            setIfNotFocused(lightREl, String(r));
            setIfNotFocused(lightGEl, String(g));
            setIfNotFocused(lightBEl, String(b));
            const bp = Number.isFinite(Number(data.ledStripBrightnessPercent)) ? Number(data.ledStripBrightnessPercent) : (Number.isFinite(Number(data.brightnessPercent)) ? Number(data.brightnessPercent) : null);
            if (bp !== null) {
                setIfNotFocused(lightBrightnessEl, String(Math.round(bp)));
            } else {
                const sameWhite = r === g && g === b;
                if (!appState.rgbModeEnabled && sameWhite) {
                    setIfNotFocused(lightBrightnessEl, String(Math.round((r / 255) * 100)));
                }
                if (!appState.rgbModeEnabled && !sameWhite) {
                    setIfNotFocused(lightBrightnessEl, String(100));
                }
            }
        }
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
    updateLightControlsState();
    updateLightScheduleLabel();
    updateLightPreview();
    mergeLocalLightState(data);

    const percent = Number.isFinite(Number(data.moisturePercent)) ? Number(data.moisturePercent) : 0;
    const raw = Number.isFinite(Number(data.moistureRaw)) ? Number(data.moistureRaw) : 0;
    const clampedPercent = Math.max(0, Math.min(100, percent));
    moisturePercentEl.textContent = `${percent}%`;
    moistureRawEl.textContent = String(raw);
    moistureBarEl.style.width = `${clampedPercent}%`;
    if (statusMoistureFillEl) {
        statusMoistureFillEl.style.width = `${clampedPercent}%`;
    }

    const remainingMs = Number.isFinite(Number(data.pumpRemainingMs)) ? Number(data.pumpRemainingMs) : 0;
    const pumpDurationMs = Number.isFinite(Number(data.pumpDurationMs)) ? Number(data.pumpDurationMs) : 0;
    statusPumpEl.textContent = pumpOn ? detail.on : detail.off;
    statusPumpDurationEl.textContent = `${formatDurationMs(pumpDurationMs)} / ${formatDurationMs(remainingMs)}`;

    const manualEnabled = typeof data.manualPumpControlEnabled === "boolean" ? data.manualPumpControlEnabled : false;
    const autoEnabled = !manualEnabled;
    statusPumpModeEl.textContent = `${manualEnabled ? detail.manualControlOn : detail.manualControlOff} | ${autoEnabled ? detail.automationOn : detail.automationOff}`;

    const cooldownMs = Number.isFinite(Number(data.autoPumpCooldownMs)) ? Number(data.autoPumpCooldownMs) : 0;
    statusPumpCooldownEl.textContent = cooldownMs > 0 ? formatDurationMs(cooldownMs) : "-";

    const threshold = Number.isFinite(Number(data.moistureThresholdPercent)) ? Number(data.moistureThresholdPercent) : null;
    statusMoistureEl.textContent = threshold === null ? `${percent}% (${raw})` : `${percent}% (${raw}) | ${detail.thresholdWord} ${threshold}%`;

    renderLightStatusFromLocalState();

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
    if (!appState.lightDraftActive && !appState.lightSliderDragging) {
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
    updateLightControlsState();
    updateLightScheduleLabel();
    mergeLocalLightState(data);
    renderLightStatusFromLocalState();
}

async function fetchGardenSettings() {
    return fetchJsonFromController("/api/garden/settings", {
        method: "GET",
        timeoutMs: STATUS_FETCH_TIMEOUT_MS,
        validate: isObjectRecord
    });
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
            lightOffMinute: timeInputValueToMinutes(lightOffTimeInputEl.value, 23 * 60),
            ledEffect: lightEffectSelectEl.value,
            ledEffectSpeedMs: Number(lightEffectSpeedInputEl.value || 1200)
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

    try {
        await postJson("/api/settings", {
            timezone: timezoneInputEl.value,
            liveRateEnabled: appState.liveRateEnabled,
            liveIntervalMs: getLiveInterval()
        });

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
        "vg_port_enabled"
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
    const savedLiveRateEnabled = localStorage.getItem(storageKeys.liveRateEnabled);
    const savedLiveInterval = localStorage.getItem(storageKeys.liveInterval);

    ipInputEl.placeholder = DEFAULT_CONTROLLER_IP;
    if (savedIp !== null) {
        ipInputEl.value = savedIp;
    } else {
        ipInputEl.value = DEFAULT_CONTROLLER_IP;
    }
    if (savedTz && Array.from(timezoneInputEl.options).some((opt) => opt.value === savedTz)) {
        timezoneInputEl.value = savedTz;
    }
    if (savedPort) portInputEl.value = savedPort;
    appState.portEnabled = savedPortEnabled === "true"
        || (savedPortEnabled === null && Boolean(savedPort) && savedPort !== "80");
    portEnabledEl.checked = appState.portEnabled;
    portInputEl.disabled = !appState.portEnabled;
    ctrlState.lastReachableBaseUrl = null;
    if (savedMode === "now" || savedMode === "manual") {
        timeModeEl.value = savedMode;
    }
    if (savedManual) manualTimeEl.value = savedManual;
    if (savedLanguage === "en" || savedLanguage === "de") {
        appState.language = savedLanguage;
    }
    appState.live = savedLive === "true";
    liveStatusToggleEl.checked = appState.live;
    appState.advancedOpen = false;
    advancedContentEl.hidden = !appState.advancedOpen;
    advancedChevronEl.textContent = appState.advancedOpen ? "▴" : "▾";
    appState.liveRateEnabled = savedLiveRateEnabled === "true";
    liveRateEnabledEl.checked = appState.liveRateEnabled;
    if (savedLiveInterval) {
        liveIntervalInputEl.value = savedLiveInterval;
    } else {
        liveIntervalInputEl.value = String(appState.liveInterval);
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
            normalized.ledStripBrightnessPercent = data.ledStrip.brightnessPercent;
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
    return fetchJsonFromController(path, {
        method: "POST",
        payload,
        timeoutMs: API_FETCH_TIMEOUT_MS,
        validate: isLikelyControllerAckPayload
    });
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
        const brightnessPercent = Number(lightBrightnessEl.value || 100);
        const baseR = appState.rgbModeEnabled ? Number(lightREl.value || 0) : 255;
        const baseG = appState.rgbModeEnabled ? Number(lightGEl.value || 0) : 255;
        const baseB = appState.rgbModeEnabled ? Number(lightBEl.value || 0) : 255;
        const targetOn = brightnessPercent > 0;
        const payload = {
            on: targetOn,
            r: baseR,
            g: baseG,
            b: baseB,
            brightnessPercent,
            effect: lightEffectSelectEl.value,
            effectSpeedMs: Number(lightEffectSpeedInputEl.value || 1200)
        };

        const response = await postJson("/api/led/effect", payload);
        mergeLocalLightState({
            ledStripOn: payload.on,
            ledStripR: getLightValues().r,
            ledStripG: getLightValues().g,
            ledStripB: getLightValues().b,
            ledEffect: payload.effect,
            ledEffectSpeedMs: payload.effectSpeedMs
        });
        mergeLocalLightState(response);
        if (typeof response.effect === "string") {
            lightEffectSelectEl.value = response.effect;
        }
        if (Number.isFinite(Number(response.effectSpeedMs))) {
            setIfNotFocused(lightEffectSpeedInputEl, String(response.effectSpeedMs));
        }
        renderLightStatusFromLocalState();
        await fetchControllerData(false);
        setConnectionState(ui[appState.language].connected, "ok");
        setStatus("Lichteinstellungen aktualisiert.", null);
    } catch (error) {
        setStatus(`Licht-Fehler: ${error.message}`, "error");
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
    ctrlState.pollInFlight = false;
}

function startPolling() {
    stopPolling();
    if (isValidIPv4(ipInputEl.value.trim())) {
        fetchControllerData(false);
    }
    ctrlState.statusTimer = setInterval(async () => {
        if (ctrlState.pollInFlight) {
            return;
        }
        ctrlState.pollInFlight = true;
        try {
            await fetchControllerData(false);
        } finally {
            ctrlState.pollInFlight = false;
        }
    }, getLiveInterval());
}

function attemptAutoConnect() {
    if (appState.live) {
        return;
    }
    if (ctrlState.fetchInFlight || ctrlState.pollInFlight) {
        return;
    }
    if (!isValidIPv4(ipInputEl.value.trim())) {
        return;
    }
    fetchControllerData(false);
}

function startAutoConnect() {
    if (ctrlState.autoConnectTimer) {
        clearInterval(ctrlState.autoConnectTimer);
    }
    ctrlState.autoConnectTimer = setInterval(attemptAutoConnect, AUTO_CONNECT_INTERVAL_MS);
    attemptAutoConnect();
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
    ctrlState.lastReachableBaseUrl = null;

    ipInputEl.value = DEFAULT_CONTROLLER_IP;
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
    liveIntervalInputEl.value = "4000";
    liveIntervalInputEl.disabled = true;
    appState.liveRateEnabled = false;
    appState.liveInterval = 4000;
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
    localLightState.hasValue = false;
    resetClockState();
}

// Read status data from known controller endpoints.
async function fetchControllerData(showFeedback = true) {
    if (ctrlState.fetchInFlight && !showFeedback) {
        return null;
    }
    ctrlState.fetchInFlight = true;

    const deviceIp = ipInputEl.value.trim();
    if (!isValidIPv4(deviceIp)) {
        setConnectionState("IP fehlt/ungueltig", "error");
        if (showFeedback) setStatus("Bitte eine gueltige IPv4-Adresse eintragen.", "warn");
        ctrlState.fetchInFlight = false;
        return null;
    }

    try {
        let data;
        try {
            data = await fetchJsonFromController("/api/status", {
                method: "GET",
                timeoutMs: STATUS_FETCH_TIMEOUT_MS,
                validate: isLikelyControllerStatusPayload
            });
        } catch {
            data = await fetchJsonFromController("/status", {
                method: "GET",
                timeoutMs: STATUS_FETCH_TIMEOUT_MS,
                validate: isLikelyControllerStatusPayload
            });
        }

        setConnectionState(ui[appState.language].connected, "ok");
        renderControllerData(data);

        if (showFeedback) {
            const now = Date.now();
            if (now - ctrlState.lastSettingsFetchMs >= 10000) {
                try {
                    const settings = await fetchGardenSettings();
                    renderGardenSettingsData(settings);
                    ctrlState.lastSettingsFetchMs = now;
                } catch {
                    // ignore settings fetch errors to keep status responsive
                }
            }
        }

        return data;
    } catch (error) {
        if (error.name === "AbortError") {
            setConnectionState("Timeout", "error");
            if (showFeedback) setStatus("Timeout beim Verbinden mit dem Controller.", "error");
        } else if (error instanceof TypeError) {
            setConnectionState("WLAN getrennt?", "error");
            if (showFeedback) setStatus("Verbindung weg (Controller-WLAN prüfen) oder CORS-Blockade.", "error");
        } else {
            setConnectionState("Keine Verbindung", "error");
            if (showFeedback) setStatus("Controller nicht erreichbar oder kein bekanntes Status-Endpoint gefunden.", "error");
        }
        return null;
    } finally {
        ctrlState.fetchInFlight = false;
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

    try {
        await postJson("/api/time", payload);

        setConnectionState(ui[appState.language].connected, "ok");
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
        syncBtnEl.disabled = false;
    }
}

// Initial setup.
initTheme();
loadTimezones();
setupSectionCollapsibles();
restoreFormState();
appState.language = appState.language === "en" ? "en" : "de";
applyLanguage(appState.language);
renderLiveClocks();
setInterval(renderLiveClocks, 1000);
if (appState.live) {
    startPolling();
}
startAutoConnect();

function bindLightRangeInteraction(rangeEl) {
    if (!rangeEl) return;
    const begin = () => {
        appState.lightSliderDragging = true;
        appState.lightDraftActive = true;
    };
    const end = () => {
        appState.lightSliderDragging = false;
    };

    rangeEl.addEventListener("pointerdown", begin);
    rangeEl.addEventListener("pointerup", end);
    rangeEl.addEventListener("pointercancel", end);
    rangeEl.addEventListener("touchstart", begin, { passive: true });
    rangeEl.addEventListener("touchend", end, { passive: true });
    rangeEl.addEventListener("change", end);
    rangeEl.addEventListener("blur", end);
}

[lightBrightnessEl, lightREl, lightGEl, lightBEl].forEach(bindLightRangeInteraction);

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
    ctrlState.lastReachableBaseUrl = null;
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
    ctrlState.lastReachableBaseUrl = null;
    setConnectionState("Nicht geprueft", "");
    resetClockState();
    renderLiveClocks();
    if (isValidIPv4(ipInputEl.value.trim())) {
        fetchControllerData(false);
    }
});
timezoneInputEl.addEventListener("change", () => {
    saveFormState();
    updateAutoNowPreview();
});
portInputEl.addEventListener("change", () => {
    ctrlState.lastReachableBaseUrl = null;
    saveFormState();
});
manualTimeEl.addEventListener("change", saveFormState);
resetButtonEl.addEventListener("click", resetSettings);
pumpToggleBtnEl.addEventListener("click", togglePump);
pumpDurationSaveBtnEl.addEventListener("click", savePumpDuration);
lightApplyBtnEl.addEventListener("click", applyLight);
gardenSettingsSaveBtnEl.addEventListener("click", saveGardenSettings);
lightScheduleSaveBtnEl.addEventListener("click", saveLightScheduleOnly);
sensorRefreshBtnEl.addEventListener("click", refreshSensors);
lightBrightnessEl.addEventListener("input", () => {
    appState.lightDraftActive = true;
    updateLightPreview();
});
if (rgbModeToggleEl) {
    rgbModeToggleEl.addEventListener("change", () => {
        appState.lightDraftActive = true;
        setRgbMode(rgbModeToggleEl.checked);
    });
}
manualPumpControlToggleEl.addEventListener("change", async () => {
    updatePumpModeLabels();
    await saveGardenSettings();
});
autoPumpEnabledToggleEl.addEventListener("change", updatePumpModeLabels);
lightScheduleEnabledToggleEl.addEventListener("change", () => {
    appState.lightDraftActive = true;
    updateLightScheduleLabel();
    updateLightControlsState();
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
    updateEffectSpeedVisibility();
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
setRgbMode(appState.rgbModeEnabled);
updateLightControlsState();
updateLightPreview();
updatePumpModeLabels();
updateLightScheduleLabel();
