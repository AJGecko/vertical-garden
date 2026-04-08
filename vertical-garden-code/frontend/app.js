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
const ctrlState = { statusTimer: null };
const clockState = {
    controllerBaseMs: null,
    controllerCapturedMs: null,
    controllerTimeZone: null,
    hasSample: false
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
    liveInterval: 500
};

const fallbackTimezones = [
    DEFAULT_TIMEZONE,
    "UTC",
    "Europe/Zurich",
    "Europe/Vienna"
];

// Resets projected controller clock values.
function resetClockState() {
    clockState.controllerBaseMs = null;
    clockState.controllerCapturedMs = null;
    clockState.controllerTimeZone = null;
    clockState.hasSample = false;
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

    if (!clockState.hasSample || clockState.controllerBaseMs === null || clockState.controllerCapturedMs === null) {
        ctrlTimeEl.textContent = "-";
        ctrlLocalEl.textContent = new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
        return;
    }

    const elapsed = now - clockState.controllerCapturedMs;
    const projectedControllerTime = new Date(clockState.controllerBaseMs + elapsed);
    ctrlTimeEl.textContent = projectedControllerTime.toLocaleString(DISPLAY_LOCALE, {
        timeZone: clockState.controllerTimeZone || timezone
    });
    ctrlLocalEl.textContent = new Date(now).toLocaleString(DISPLAY_LOCALE, { timeZone: timezone });
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
    const timezone = data.timezone || data.tz || "-";
    const local = data.localTime || data.localDateTime || data.time || data.datetime || data.isoTime || null;

    let displayTime = "-";
    let parsedControllerDate = null;
    if (local) {
        const parsed = new Date(local);
        displayTime = Number.isNaN(parsed.getTime())
            ? String(local)
            : parsed.toLocaleString(DISPLAY_LOCALE, { timeZone: timezone !== "-" ? timezone : undefined });
        parsedControllerDate = Number.isNaN(parsed.getTime()) ? null : parsed;
    }

    if (parsedControllerDate) {
        clockState.controllerBaseMs = parsedControllerDate.getTime();
        clockState.controllerCapturedMs = Date.now();
        clockState.controllerTimeZone = timezone !== "-" ? timezone : null;
        clockState.hasSample = true;
    } else {
        resetClockState();
        ctrlTimeEl.textContent = displayTime;
    }

    ctrlTimezoneEl.textContent = timezone;
    ctrlUpdatedEl.textContent = new Date().toLocaleString(DISPLAY_LOCALE);
    renderLiveClocks();
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
        `${baseUrl}/api/time`,
        `${baseUrl}/api/status`,
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

// Initial render for advanced section and live status
advancedContentEl.hidden = true;
advancedChevronEl.textContent = "▾";
liveStatusTitleEl.textContent = ui[appState.language].liveStatus;
liveStatusToggleTextEl.textContent = appState.live ? ui[appState.language].liveOn : ui[appState.language].liveOff;
updateLiveRateState();
