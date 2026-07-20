// ClickyWeather PKJS - Open-Meteo fetcher
// No API key required. Fetches forecast + air-quality and posts AppMessage.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var currentVersion = require('./version');
var messageKeys = require('message_keys');
var weatherUtils = require('./weather-utils');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

var ALERT_CAT = weatherUtils.ALERT_CAT;
var degToCompass = weatherUtils.degToCompass;
var isNewerVersion = weatherUtils.isNewerVersion;
var mapWeatherCode = weatherUtils.mapWeatherCode;
var parseCoordinateOverride = weatherUtils.parseCoordinateOverride;
var payloadValue = weatherUtils.payloadValue;
var validCoordinates = weatherUtils.validCoordinates;

var UPDATE_CHECK_URL =
  'https://api.github.com/repos/tylxr59/ClickyWeather/releases/latest';
var UPDATE_CHECK_DEFAULT_INTERVAL_SECS = 24 * 60 * 60;
var UPDATE_CHECK_EVERY_WEATHER_FETCH = -1;
var UPDATE_CHECK_INTERVAL_KEY = 'appUpdateCheckInterval';
var UPDATE_CHECK_TIME_KEY = 'updateCheckTime';
var UPDATE_AVAILABLE_KEY = 'updateAvailable';
var UPDATE_CHECK_VERSION_KEY = 'updateCheckInstalledVersion';
var updateCheckInFlight = false;
var activeWeatherSeq = null;

var FETCH_ERROR = {
  NONE: 0,
  NETWORK: 1,
  LOCATION: 2,
  TIMEOUT: 3
};

function use24h() {
  var format = localStorage.getItem('timeFormat') || '0';
  if (format === '1') return false;
  if (format === '2') return true;
  return localStorage.getItem('clockIs24h') === '1';
}

function fmtTime12(iso) {
  if (!iso) return '';
  var t = iso.split('T')[1] || '';
  var parts = t.split(':');
  var h = parseInt(parts[0], 10);
  var m = parts[1] || '00';
  if (use24h()) {
    return (h < 10 ? '0' + h : h) + ':' + m;
  }
  var ampm = h >= 12 ? 'PM' : 'AM';
  h = h % 12; if (h === 0) h = 12;
  return h + ':' + m + ' ' + ampm;
}

// Moon phase from Julian-date formula. Open-Meteo does not provide
// moon_phase / moonrise / moonset (verified 2026-05). Reference new
// moon: 2000-01-06 18:14 UTC = JD 2451550.1. Synodic month = 29.5305889.
// Returns { phase: 0..7 enum, illum: 0..100, name1, name2 }.
function computeMoonPhase(date) {
  var ms = date.getTime();
  var jd = ms / 86400000.0 + 2440587.5;
  var p = (jd - 2451550.1) / 29.530588853;
  p = p - Math.floor(p); // 0..1
  // Illumination via cosine of phase angle.
  var illum = Math.round((1 - Math.cos(p * 2 * Math.PI)) * 50);
  // Phase enum + names. Bands per standard astronomy convention.
  var phase, name1, name2;
  if      (p < 0.03)  { phase = 0; name1 = 'NEW';     name2 = 'MOON'; }
  else if (p < 0.22)  { phase = 1; name1 = 'WAXING';  name2 = 'CRESCENT'; }
  else if (p < 0.28)  { phase = 2; name1 = 'FIRST';   name2 = 'QUARTER'; }
  else if (p < 0.47)  { phase = 3; name1 = 'WAXING';  name2 = 'GIBBOUS'; }
  else if (p < 0.53)  { phase = 4; name1 = 'FULL';    name2 = 'MOON'; }
  else if (p < 0.72)  { phase = 5; name1 = 'WANING';  name2 = 'GIBBOUS'; }
  else if (p < 0.78)  { phase = 6; name1 = 'LAST';    name2 = 'QUARTER'; }
  else if (p < 0.97)  { phase = 7; name1 = 'WANING';  name2 = 'CRESCENT'; }
  else                { phase = 0; name1 = 'NEW';     name2 = 'MOON'; }
  return { phase: phase, illum: illum, name1: name1, name2: name2 };
}

function getUnits() {
  return localStorage.getItem('units') === 'metric' ? 'metric' : 'imperial';
}

function getUseDewPoint() {
  return localStorage.getItem('useDewPoint') === '1';
}

// Returns true when coordinates are within NWS-covered US regions.
function isUS(lat, lon) {
  var conus = lat >= 24 && lat <= 50 && lon >= -125 && lon <= -66;
  var alaska = lat >= 51 && lat <= 72 && lon >= -170 && lon <= -130;
  var hawaii = lat >= 18 && lat <= 23 && lon >= -161 && lon <= -154;
  return conus || alaska || hawaii;
}


// Returns true when coordinates are within continental Europe + UK +
// Scandinavia. Open-Meteo CAMS pollen data is reliable inside this box;
// outside it the watch currently hides pollen data.
function isEurope(lat, lon) {
  return lat >= 35 && lat <= 72 && lon >= -25 && lon <= 45;
}

// Convert Open-Meteo pollen values (grains/m³) to the 0-5 UPI-style
// scale historically used by the pollen badge, using European Aeroallergen
// Network (EAN) category thresholds per pollen type. Returns -1 when
// all inputs are null (region not covered by CAMS).
//
// EAN bands (grains/m³) used here, normalized to the Google UPI 0..5
// scale (0=None, 1=Very Low, 2=Low, 3=Moderate, 4=High, 5=Very High):
//   Grass:   0 | 1-5   | 6-20  | 21-50  | 51-200 | >200
//   Tree:    0 | 1-15  | 16-50 | 51-100 | 101-300| >300  (birch/alder)
//   Weed:    0 | 1-5   | 6-15  | 16-50  | 51-200 | >200  (ragweed/mugwort)
function pollenGrainsToUpi(grass, birch, alder, ragweed, mugwort, olive) {
  function grassScale(v)   {
    if (v === null || v === undefined) return -1;
    if (v <= 0)   return 0; if (v <= 5)   return 1;
    if (v <= 20)  return 2; if (v <= 50)  return 3;
    if (v <= 200) return 4; return 5;
  }
  function treeScale(v) {
    if (v === null || v === undefined) return -1;
    if (v <= 0)   return 0; if (v <= 15)  return 1;
    if (v <= 50)  return 2; if (v <= 100) return 3;
    if (v <= 300) return 4; return 5;
  }
  function weedScale(v) {
    if (v === null || v === undefined) return -1;
    if (v <= 0)   return 0; if (v <= 5)   return 1;
    if (v <= 15)  return 2; if (v <= 50)  return 3;
    if (v <= 200) return 4; return 5;
  }
  var vals = [
    grassScale(grass), treeScale(birch),   treeScale(alder),
    weedScale(ragweed), weedScale(mugwort), treeScale(olive),
  ];
  var max = -1;
  for (var i = 0; i < vals.length; i++) {
    if (vals[i] > max) max = vals[i];
  }
  return max;
}

// Phase 11: Golden / Blue hour computation.
//
// Compact port of Vladimir Agafonkin's SunCalc (BSD-2). Computes sun
// times for arbitrary altitude angles. We use:
//   -6°    civil twilight (blue hour outer boundary)
//   -0.833° apparent sunrise/sunset (atmospheric refraction)
//    +6°   sun "high" boundary (golden hour upper edge)
//
// Morning chronology: blueHour.rise → sunrise.rise → goldenHour.rise
// Evening chronology: goldenHour.set → sunrise.set → blueHour.set
//
// We send four "milestone start" timestamps:
//   blue_am = blueHour.rise   (morning blue hour begins)
//   gold_am = sunrise.rise    (morning golden hour begins, sunrise)
//   gold_pm = goldenHour.set  (evening golden hour begins)
//   blue_pm = sunrise.set     (evening blue hour begins, sunset)
function computeGoldenHour(date, lat, lng, utcOffsetSec) {
  var rad = Math.PI / 180;
  var dayMs = 86400000;
  var J1970 = 2440588;
  var J2000 = 2451545;

  function toJulian(d) { return d.valueOf() / dayMs - 0.5 + J1970; }
  function fromJulian(j) { return new Date((j + 0.5 - J1970) * dayMs); }
  function toDays(d) { return toJulian(d) - J2000; }

  var e = rad * 23.4397;
  function declination(l, b) {
    return Math.asin(Math.sin(b) * Math.cos(e) +
                     Math.cos(b) * Math.sin(e) * Math.sin(l));
  }
  function solarMeanAnomaly(d) { return rad * (357.5291 + 0.98560028 * d); }
  function eclipticLongitude(M) {
    var C = rad * (1.9148 * Math.sin(M) + 0.02 * Math.sin(2 * M) +
                   0.0003 * Math.sin(3 * M));
    var P = rad * 102.9372;
    return M + C + P + Math.PI;
  }
  function julianCycle(d, lw) { return Math.round(d - 0.0009 - lw / (2 * Math.PI)); }
  function approxTransit(Ht, lw, n) { return 0.0009 + (Ht + lw) / (2 * Math.PI) + n; }
  function solarTransitJ(ds, M, L) {
    return J2000 + ds + 0.0053 * Math.sin(M) - 0.0069 * Math.sin(2 * L);
  }
  function hourAngle(h, phi, d) {
    return Math.acos((Math.sin(h) - Math.sin(phi) * Math.sin(d)) /
                     (Math.cos(phi) * Math.cos(d)));
  }
  function getSetJ(h, lw, phi, dec, n, M, L) {
    var w = hourAngle(h, phi, dec);
    var a = approxTransit(w, lw, n);
    return solarTransitJ(a, M, L);
  }

  var lw = rad * -lng;
  var phi = rad * lat;
  var d = toDays(date);
  var n = julianCycle(d, lw);
  var ds = approxTransit(0, lw, n);
  var M = solarMeanAnomaly(ds);
  var L = eclipticLongitude(M);
  var dec = declination(L, 0);
  var Jnoon = solarTransitJ(ds, M, L);

  function timesForAngle(angle) {
    var Jset = getSetJ(angle * rad, lw, phi, dec, n, M, L);
    var Jrise = Jnoon - (Jset - Jnoon);
    return { rise: fromJulian(Jrise), set: fromJulian(Jset) };
  }

  var sunrise = timesForAngle(-0.833);
  var blue = timesForAngle(-6);
  var gold = timesForAngle(6);

  // hourAngle can return NaN at extreme latitudes / polar day-night.
  // Detect via isNaN on the resulting Date and bail out gracefully.
  //
  // Date math note: SunCalc's fromJulian() returns a UTC-anchored Date.
  // The PKJS runtime's host timezone is unpredictable (often UTC on the
  // phone bridge), so we must NOT use d.getHours()/getMinutes() — they
  // would return host-local time, not the user's local time. Instead we
  // shift by Open-Meteo's `utc_offset_seconds` (which is exact for the
  // forecast point's tz, including DST) and read getUTC* from the
  // shifted timestamp.
  function fmt(d) {
    if (!d || isNaN(d.getTime())) return '--:--';
    var shifted = new Date(d.getTime() + (utcOffsetSec || 0) * 1000);
    var h = shifted.getUTCHours();
    var m = shifted.getUTCMinutes();
    var mm = m < 10 ? '0' + m : m;
    if (use24h()) {
      return (h < 10 ? '0' + h : h) + ':' + mm;
    }
    var ampm = h >= 12 ? 'PM' : 'AM';
    h = h % 12; if (h === 0) h = 12;
    return h + ':' + mm + ' ' + ampm;
  }

  return {
    BlueAm: fmt(blue.rise),
    GoldAm: fmt(sunrise.rise),
    GoldPm: fmt(gold.set),
    BluePm: fmt(sunrise.set)
  };
}

function xhr(url, cb) {
  var req = new XMLHttpRequest();
  var finished = false;
  var timer = setTimeout(function() {
    if (finished) return;
    finished = true;
    try { req.abort(); } catch (e) {}
    cb(new Error('xhr timeout'));
  }, 15000);

  function done(err, data) {
    if (finished) return;
    finished = true;
    clearTimeout(timer);
    cb(err, data);
  }

  req.open('GET', url, true);
  req.timeout = 15000;
  req.onload = function() {
    if (req.status >= 200 && req.status < 300) {
      try { done(null, JSON.parse(req.responseText)); }
      catch (e) { done(e); }
    } else {
      done(new Error('HTTP ' + req.status));
    }
  };
  req.onerror = function() { done(new Error('xhr error')); };
  req.ontimeout = function() { done(new Error('xhr timeout')); };
  req.send();
}

function cachedUpdateAvailable() {
  return localStorage.getItem(UPDATE_AVAILABLE_KEY) === '1';
}

function validUpdateCheckInterval(value) {
  return value === 0 || value === UPDATE_CHECK_EVERY_WEATHER_FETCH ||
         value === 21600 || value === 43200 || value === 86400 ||
         value === 259200 || value === 604800;
}

function claySettingValue(name) {
  try {
    var saved = JSON.parse(localStorage.getItem('clay-settings') || '{}');
    var value = saved[name];
    if (value && value.value !== undefined) value = value.value;
    return value;
  } catch (err) {
    console.log('saved setting read skipped: ' + err.message);
    return undefined;
  }
}

function getUpdateCheckInterval() {
  var value = localStorage.getItem(UPDATE_CHECK_INTERVAL_KEY);
  if (value === null) value = claySettingValue('AppUpdateCheckInterval');
  value = parseInt(value, 10);
  return validUpdateCheckInterval(value)
    ? value : UPDATE_CHECK_DEFAULT_INTERVAL_SECS;
}

function setUpdateCheckInterval(value) {
  value = parseInt(value, 10);
  if (!validUpdateCheckInterval(value)) {
    value = UPDATE_CHECK_DEFAULT_INTERVAL_SECS;
  }
  localStorage.setItem(UPDATE_CHECK_INTERVAL_KEY, String(value));
}

function prepareUpdateCheckCache() {
  var cachedVersion = localStorage.getItem(UPDATE_CHECK_VERSION_KEY);
  if (cachedVersion === currentVersion) return;

  // PKJS storage can survive an app upgrade or deliberate downgrade. A cached
  // result from another installed version must not leak into this version.
  localStorage.setItem(UPDATE_CHECK_VERSION_KEY, currentVersion);
  localStorage.removeItem(UPDATE_CHECK_TIME_KEY);
  localStorage.removeItem(UPDATE_AVAILABLE_KEY);
}

function sendUpdateAvailability(available) {
  Pebble.sendAppMessage({
    UpdateAvailable: available ? 1 : 0
  }, function() {
    console.log('update availability sent');
  }, function(e) {
    console.log('update availability send fail: ' + JSON.stringify(e));
  });
}

function checkForUpdate(trigger, force) {
  var intervalSecs = getUpdateCheckInterval();
  if (!force && intervalSecs === 0) {
    localStorage.removeItem(UPDATE_CHECK_TIME_KEY);
    localStorage.setItem(UPDATE_AVAILABLE_KEY, '0');
    sendUpdateAvailability(false);
    return;
  }

  if (!force && intervalSecs === UPDATE_CHECK_EVERY_WEATHER_FETCH &&
      trigger !== 'weather') {
    sendUpdateAvailability(cachedUpdateAvailable());
    return;
  }

  var now = Date.now();
  var lastCheck = parseInt(localStorage.getItem(UPDATE_CHECK_TIME_KEY), 10) || 0;

  if (!force && intervalSecs > 0 &&
      now - lastCheck < intervalSecs * 1000) {
    sendUpdateAvailability(cachedUpdateAvailable());
    return;
  }
  if (updateCheckInFlight) return;

  updateCheckInFlight = true;
  xhr(UPDATE_CHECK_URL, function(err, release) {
    updateCheckInFlight = false;
    if (err || !release || !release.tag_name) {
      console.log('update check failed: ' +
                  (err ? err.message : 'missing release tag'));
      sendUpdateAvailability(cachedUpdateAvailable());
      return;
    }

    var available = isNewerVersion(release.tag_name, currentVersion);
    // Only a successful response starts the selected interval. A temporary
    // GitHub failure can recover on a later app or weather event.
    localStorage.setItem(UPDATE_CHECK_TIME_KEY, String(Date.now()));
    localStorage.setItem(UPDATE_AVAILABLE_KEY, available ? '1' : '0');
    console.log('update check: installed=' + currentVersion +
                ' latest=' + release.tag_name +
                ' available=' + available);
    sendUpdateAvailability(available);
  });
}

function isActiveWeatherRequest(seq) {
  return activeWeatherSeq === seq;
}

function sendFetchError(reason, seq, code) {
  if (!isActiveWeatherRequest(seq)) return;
  console.log('fetch failed: ' + reason);
  Pebble.sendAppMessage({
    FetchError: code || FETCH_ERROR.NETWORK,
    ResponseSeq: seq
  }, function() {
    if (isActiveWeatherRequest(seq)) activeWeatherSeq = null;
    console.log('fetch error sent');
  }, function(e) {
    if (isActiveWeatherRequest(seq)) activeWeatherSeq = null;
    console.log('fetch error send fail: ' + JSON.stringify(e));
  });
}

function buildWeatherMessage(seq, lat, lon, data, aqd, alertResult) {
  if (!isActiveWeatherRequest(seq)) return;
  var units = getUnits();
  var msg = {};
  try {
        var cur = data && data.current || {};
        var daily = data && data.daily || {};
        var hourly = data && data.hourly || {};
        if (typeof cur.temperature_2m !== 'number') {
          throw new Error('missing current temperature');
        }
        msg.Temp = Math.round(cur.temperature_2m);
        msg.FeelsLike = typeof cur.apparent_temperature === 'number'
          ? Math.round(cur.apparent_temperature) : msg.Temp;
        msg.Humidity = typeof cur.relative_humidity_2m === 'number'
          ? Math.round(cur.relative_humidity_2m) : -1;
        msg.DewPoint = typeof cur.dew_point_2m === 'number'
          ? Math.round(cur.dew_point_2m) : -1;
        msg.UseDewPoint = getUseDewPoint() ? 1 : 0;
        msg.Wind = typeof cur.wind_speed_10m === 'number'
          ? Math.round(cur.wind_speed_10m) : -1;
        msg.WindDir = degToCompass(cur.wind_direction_10m || 0);
        msg.WindGust = Math.round(cur.wind_gusts_10m || 0);
        msg.Condition = mapWeatherCode(cur.weather_code);
        if (daily.temperature_2m_max && daily.temperature_2m_min &&
            typeof daily.temperature_2m_max[0] === 'number' &&
            typeof daily.temperature_2m_min[0] === 'number') {
          msg.High = Math.round(daily.temperature_2m_max[0]);
          msg.Low = Math.round(daily.temperature_2m_min[0]);
        } else {
          msg.High = msg.Temp;
          msg.Low = msg.Temp;
        }
        if (daily.sunrise && daily.sunrise.length) {
          msg.Sunrise = fmtTime12(daily.sunrise[0]);
          msg.Sunset = fmtTime12(daily.sunset[0]);
        } else {
          msg.Sunrise = '';
          msg.Sunset = '';
        }
        // UV semantics:
        //   msg.UV    — current UV (live gauge value)
        //   msg.UVMax — today's forecast peak (subtitle "PEAK n")
        // Fall back to daily peak if `current.uv_index` is missing on
        // older API responses, so we never regress to undefined.
        var dailyMax = (daily.uv_index_max &&
                        typeof daily.uv_index_max[0] === 'number')
                       ? daily.uv_index_max[0] : null;
        msg.UV = -1;
        if (typeof cur.uv_index === 'number') {
          msg.UV = Math.round(cur.uv_index);
        } else if (dailyMax !== null) {
          msg.UV = Math.round(dailyMax);
        }
        msg.UVMax = dailyMax !== null ? Math.round(dailyMax) : -1;
        var p = hourly.precipitation_probability || [];
        var times = hourly.time || [];
        // Find the index of the current hour in the hourly arrays so the
        // 5 bars truly represent "Now / +1h / +2h / +3h / +4h" instead of
        // starting at midnight. Open-Meteo returns local-tz timestamps
        // like "2026-05-06T14:00" when timezone=auto.
        var startIdx = 0;
        if (times.length) {
          var nowKey;
          if (cur.time) {
            nowKey = cur.time.slice(0, 13) + ':00';
          } else {
            var now = new Date();
            var pad = function(n) { return n < 10 ? '0' + n : '' + n; };
            nowKey = now.getFullYear() + '-' + pad(now.getMonth() + 1) +
                     '-' + pad(now.getDate()) + 'T' + pad(now.getHours()) + ':00';
          }
          for (var k = 0; k < times.length; k++) {
            if (times[k] === nowKey) { startIdx = k; break; }
          }
        }
        for (var i = 0; i < 5; i++) {
          msg['Precip' + i] = Math.round(p[startIdx + i] || 0);
        }
        // Normalize Open-Meteo's millimetres to tenths of the selected display
        // unit before sending, keeping floats off the watch.
        var precipMm = (hourly.precipitation || [])[startIdx] || 0;
        msg.PrecipAmount = weatherUtils.precipMmToDisplayX10(precipMm, units);
        msg.Units = units === 'metric' ? 1 : 0;
        msg.LastUpdated = Math.floor(Date.now() / 1000);
        msg.FetchError = 0;
        msg.ResponseSeq = seq;
        msg.UpdateAvailable = cachedUpdateAvailable() ? 1 : 0;

        // Phase 10A: Next 6 Hours (offsets +1h..+6h from current hour).
        var temps = hourly.temperature_2m || [];
        var codes = hourly.weather_code || [];
        var hWind  = hourly.wind_speed_10m || [];
        var hWdir  = hourly.wind_direction_10m || [];
        var hPrcp  = hourly.precipitation || [];
        var hUv    = hourly.uv_index || [];
        var alert = -1;
        for (var j = 0; j <= 6; j++) {
          if (Math.round((hPrcp[startIdx + j] || 0) * 10) > 0) {
            alert = j === 0 ? 15 : j * 60;
            break;
          }
        }
        msg.RainAlertMinutes = alert;
        for (var hi = 1; hi <= 6; hi++) {
          var idx = startIdx + hi;
          var hourLabel = '';
          if (times[idx]) {
            var hh = parseInt(times[idx].split('T')[1].split(':')[0], 10);
            if (use24h()) {
              hourLabel = String(hh);
            } else {
              var ampm = hh >= 12 ? 'PM' : 'AM';
              hh = hh % 12; if (hh === 0) hh = 12;
              hourLabel = hh + ' ' + ampm;
            }
          }
          msg['Hour' + hi + 'Label'] = hourLabel;
          msg['Hour' + hi + 'Temp']  = Math.round(temps[idx] || 0);
          msg['Hour' + hi + 'Cond']  = mapWeatherCode(codes[idx] || 0);
          msg['Hour' + hi + 'Pop']   = Math.round(p[idx] || 0);
          // Wind speed in selected unit (mph/kmh), rounded to integer.
          msg['Hour' + hi + 'Wind']    = Math.round(hWind[idx] || 0);
          msg['Hour' + hi + 'WindDir'] = degToCompass(hWdir[idx] || 0);
          msg['Hour' + hi + 'Precip'] = weatherUtils.precipMmToDisplayX10(
            hPrcp[idx] || 0, units);
          msg['Hour' + hi + 'Uv'] = typeof hUv[idx] === 'number'
            ? Math.round(hUv[idx]) : -1;
        }

        // Phase 10B: Week Ahead (today + next 4 days = 5 total).
        var dayCodes = daily.weather_code || [];
        var dayHigh  = daily.temperature_2m_max || [];
        var dayLow   = daily.temperature_2m_min || [];
        var dayPop   = daily.precipitation_probability_max || [];
        var dayTimes = daily.time || [];
        var dayNames = ['SUN','MON','TUE','WED','THU','FRI','SAT'];
        for (var di = 0; di < 5; di++) {
          var lbl = '';
          if (dayTimes[di]) {
            var dt = new Date(dayTimes[di] + 'T00:00');
            lbl = dayNames[dt.getDay()];
          }
          msg['Day' + di + 'Label'] = lbl;
          msg['Day' + di + 'High']  = Math.round(dayHigh[di] || 0);
          msg['Day' + di + 'Low']   = Math.round(dayLow[di]  || 0);
          msg['Day' + di + 'Cond']  = mapWeatherCode(dayCodes[di] || 0);
          msg['Day' + di + 'Pop']   = Math.round(dayPop[di]  || 0);
        }

        // Phase 7: Moon phase computed locally (Open-Meteo lacks this).
        var moon = computeMoonPhase(new Date());
        msg.MoonPhase = moon.phase;
        msg.MoonIllum = moon.illum;
        msg.MoonName1 = moon.name1;
        msg.MoonName2 = moon.name2;

        // Phase 11: Golden / Blue hour times computed locally.
        // utc_offset_seconds is exact for the forecast point's tz
        // (including DST), and avoids relying on the unpredictable
        // PKJS runtime timezone.
        var tzOffset = (typeof data.utc_offset_seconds === 'number')
                       ? data.utc_offset_seconds : 0;
        var gh = computeGoldenHour(new Date(), lat, lon, tzOffset);
        msg.BlueAm = gh.BlueAm;
        msg.GoldAm = gh.GoldAm;
        msg.GoldPm = gh.GoldPm;
        msg.BluePm = gh.BluePm;

        // Always send explicit AQ and pollen sentinels so values from an older
        // location or a partial request cannot appear current.
        var pollenLevel = -1;
        if (isEurope(lat, lon) && aqd && aqd.current) {
          var aqc = aqd.current;
          var euUpi = pollenGrainsToUpi(
            aqc.grass_pollen,  aqc.birch_pollen,  aqc.alder_pollen,
            aqc.ragweed_pollen, aqc.mugwort_pollen, aqc.olive_pollen
          );
          if (euUpi >= 0) {
            pollenLevel = euUpi;
          }
        }
        var airFields = weatherUtils.airQualityFields(aqd, pollenLevel);
        for (var airKey in airFields) {
          if (airFields.hasOwnProperty(airKey)) msg[airKey] = airFields[airKey];
        }

        msg.AlertActive = alertResult.active;
        msg.AlertCategory = alertResult.category;

        Pebble.sendAppMessage(msg, function() {
          if (!isActiveWeatherRequest(seq)) return;
          localStorage.setItem('lastFetchAt', String(Date.now()));
          activeWeatherSeq = null;
          console.log('weather sent');
        }, function(e) {
          if (isActiveWeatherRequest(seq)) activeWeatherSeq = null;
          console.log('send fail: ' + JSON.stringify(e));
        });
  } catch (e) {
    sendFetchError('parse ' + e.message, seq, FETCH_ERROR.NETWORK);
  }
}

function fetchWeather(seq, lat, lon) {
  if (!isActiveWeatherRequest(seq)) return;
  if (!validCoordinates(lat, lon)) {
    sendFetchError('invalid coordinates', seq, FETCH_ERROR.LOCATION);
    return;
  }

  checkForUpdate('weather', false);

  var units = getUnits();
  var tempUnit = units === 'metric' ? 'celsius' : 'fahrenheit';
  var windUnit = units === 'metric' ? 'kmh' : 'mph';
  var fc = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat + '&longitude=' + lon +
    '&current=temperature_2m,apparent_temperature,relative_humidity_2m,dew_point_2m,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m,uv_index' +
    '&hourly=temperature_2m,weather_code,precipitation_probability,wind_speed_10m,wind_direction_10m,precipitation,uv_index' +
    '&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,uv_index_max' +
    '&temperature_unit=' + tempUnit + '&wind_speed_unit=' + windUnit +
    '&timezone=auto&forecast_days=5';
  var aq = 'https://air-quality-api.open-meteo.com/v1/air-quality' +
    '?latitude=' + lat + '&longitude=' + lon +
    '&current=us_aqi,pm2_5,pm10,ozone,nitrogen_dioxide,grass_pollen,birch_pollen,alder_pollen,ragweed_pollen,mugwort_pollen,olive_pollen' +
    '&timezone=auto';

  var pending = isUS(lat, lon) ? 3 : 2;
  var forecastError = null;
  var forecastData = null;
  var airData = null;
  var alertResult = { active: 0, category: ALERT_CAT.UNKNOWN };

  function partDone() {
    pending--;
    if (pending !== 0 || !isActiveWeatherRequest(seq)) return;
    if (forecastError) {
      sendFetchError('forecast ' + forecastError.message, seq, FETCH_ERROR.NETWORK);
      return;
    }
    buildWeatherMessage(seq, lat, lon, forecastData, airData, alertResult);
  }

  xhr(fc, function(err, data) {
    forecastError = err;
    forecastData = data;
    partDone();
  });
  xhr(aq, function(err, data) {
    if (!err) airData = data;
    partDone();
  });
  if (isUS(lat, lon)) {
    var nwsUrl = 'https://api.weather.gov/alerts/active?point=' +
                 lat + '%2C' + lon + '&status=actual&message_type=alert';
    xhr(nwsUrl, function(err, data) {
      alertResult = weatherUtils.classifyNwsResponse(err, data);
      partDone();
    });
  }
}

function locateAndFetch(seq) {
  if (isActiveWeatherRequest(seq)) {
    console.log('weather request already in flight, skipping duplicate');
    return;
  }
  activeWeatherSeq = seq;
  var overrideValue = localStorage.getItem('locationOverride');
  var override = parseCoordinateOverride(overrideValue);
  if (override) {
    fetchWeather(seq, override.lat, override.lon);
    return;
  }
  if (overrideValue) {
    sendFetchError('invalid location override', seq, FETCH_ERROR.LOCATION);
    return;
  }
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      if (!isActiveWeatherRequest(seq)) return;
      if (validCoordinates(pos.coords.latitude, pos.coords.longitude)) {
        fetchWeather(seq, pos.coords.latitude, pos.coords.longitude);
      } else {
        sendFetchError('invalid gps coordinates', seq, FETCH_ERROR.LOCATION);
      }
    },
    function(err) {
      sendFetchError('geolocation ' + err.message, seq, FETCH_ERROR.LOCATION);
    },
    { timeout: 15000, maximumAge: 600000 }
  );
}

function savedValue(saved, name) {
  var value = saved[name];
  if (value && value.value !== undefined) return value.value;
  return value;
}

function savedBool(value) {
  return value === true || value === 1 || value === '1' || value === 'true';
}

function restoreSavedSettings(done) {
  try {
    var saved = JSON.parse(localStorage.getItem('clay-settings') || '{}');
    var msg = {};
    var value;

    value = savedValue(saved, 'Units');
    if (value !== undefined) {
      var metric = parseInt(value, 10) === 1;
      localStorage.setItem('units', metric ? 'metric' : 'imperial');
      msg.Units = metric ? 1 : 0;
    }
    value = savedValue(saved, 'UseDewPoint');
    if (value !== undefined) {
      localStorage.setItem('useDewPoint', savedBool(value) ? '1' : '0');
      msg.UseDewPoint = savedBool(value) ? 1 : 0;
    }
    value = savedValue(saved, 'TimeFormat');
    if (value !== undefined) {
      localStorage.setItem('timeFormat', String(parseInt(value, 10) || 0));
      msg.TimeFormat = parseInt(value, 10) || 0;
    }
    value = savedValue(saved, 'LocationOverride');
    if (value) localStorage.setItem('locationOverride', String(value));
    else localStorage.removeItem('locationOverride');

    value = savedValue(saved, 'Theme');
    if (value !== undefined) msg.Theme = parseInt(value, 10) || 0;
    value = savedValue(saved, 'LoopNavigation');
    if (value !== undefined) msg.LoopNavigation = savedBool(value) ? 1 : 0;
    value = savedValue(saved, 'AnimationsEnabled');
    if (value !== undefined) msg.AnimationsEnabled = savedBool(value) ? 1 : 0;
    value = savedValue(saved, 'BackgroundUpdateInterval');
    if (value !== undefined) {
      msg.BackgroundUpdateInterval = parseInt(value, 10) || 0;
    }
    value = savedValue(saved, 'AppUpdateCheckInterval');
    if (value !== undefined) setUpdateCheckInterval(value);

    var enabledMask = (1 << 9) - 1;
    var sawToggle = false;
    for (var i = 0; i < 9; i++) {
      value = savedValue(saved, 'Toggle' + i);
      if (value !== undefined) {
        sawToggle = true;
        if (savedBool(value)) enabledMask |= (1 << i);
        else enabledMask &= ~(1 << i);
        localStorage.setItem('toggle' + i, savedBool(value) ? '1' : '0');
      }
    }
    if (sawToggle) msg.EnabledMask = enabledMask;

    if (Object.keys(msg).length) {
      Pebble.sendAppMessage(msg, done, done);
    } else {
      done();
    }
  } catch (err) {
    console.log('settings restore skipped: ' + err.message);
    done();
  }
}

Pebble.addEventListener('ready', function() {
  console.log('ClickyWeather PKJS ready');
  prepareUpdateCheckCache();
  restoreSavedSettings(function() {
    checkForUpdate('ready', false);
  });
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = (e && e.payload) || {};
  var clockIs24h = payloadValue(payload, 'ClockIs24h', undefined, messageKeys);
  var refresh = payloadValue(payload, 'LastUpdated', undefined, messageKeys);
  var seq = payloadValue(payload, 'RequestSeq', 0, messageKeys);
  if (clockIs24h !== undefined) {
    localStorage.setItem('clockIs24h', clockIs24h ? '1' : '0');
  }
  if (refresh !== undefined) {
    locateAndFetch(parseInt(seq, 10) || 0);
  }
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

function weatherRelevantSnapshot() {
  return [
    localStorage.getItem('units'),
    localStorage.getItem('useDewPoint'),
    localStorage.getItem('timeFormat'),
    localStorage.getItem('locationOverride')
  ].join('|');
}

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;
  var beforeSave = weatherRelevantSnapshot();
  var previousUpdateInterval = getUpdateCheckInterval();
  var dict = clay.getSettings(e.response, false);
  var checkForAppUpdate = dict.CheckForAppUpdate !== undefined &&
    !!dict.CheckForAppUpdate.value;
  if (dict.Units !== undefined) {
    // Clay radiogroup values come back as strings ("0"/"1"), so coerce
    // before comparing. Prior versions used `=== 1` which always failed
    // and silently pinned the app to imperial.
    localStorage.setItem('units',
      parseInt(dict.Units.value, 10) === 1 ? 'metric' : 'imperial');
  }
  if (dict.UseDewPoint !== undefined) {
    localStorage.setItem('useDewPoint',
      dict.UseDewPoint.value ? '1' : '0');
  }
  if (dict.TimeFormat !== undefined) {
    localStorage.setItem('timeFormat',
      String(parseInt(dict.TimeFormat.value, 10) || 0));
  }
  if (dict.LocationOverride !== undefined && dict.LocationOverride.value) {
    localStorage.setItem('locationOverride', dict.LocationOverride.value);
  } else {
    localStorage.removeItem('locationOverride');
  }
  if (dict.AppUpdateCheckInterval !== undefined) {
    setUpdateCheckInterval(dict.AppUpdateCheckInterval.value);
  }
  
  // Handle card toggle settings and build EnabledMask.
  // Each bit in EnabledMask represents a card's enabled state (bit 0 = Toggle0, etc).
  var enabledMask = (1 << 9) - 1;
  for (var i = 0; i < 9; i++) {
    var key = 'Toggle' + i;
    if (dict[key] !== undefined) {
      if (dict[key].value) enabledMask |= (1 << i);
      else enabledMask &= ~(1 << i);
      localStorage.setItem('toggle' + i, dict[key].value ? '1' : '0');
    }
  }
  
  var msg = clay.getSettings(e.response);
  // The generic button uses a hidden one-shot Clay value. Reset it after
  // serialization so an ordinary later save does not request another check.
  clay.setSettings('CheckForAppUpdate', 0);
  msg.EnabledMask = enabledMask;
  var needsFetch = weatherRelevantSnapshot() !== beforeSave;
  msg.RefreshWeather = needsFetch ? 1 : 0;
  var updateIntervalChanged = getUpdateCheckInterval() !== previousUpdateInterval;
  if (updateIntervalChanged || checkForAppUpdate) {
    localStorage.removeItem(UPDATE_CHECK_TIME_KEY);
  }
  function afterSave() {
    if (checkForAppUpdate) {
      checkForUpdate('manual', true);
    } else if (updateIntervalChanged) {
      checkForUpdate('settings', false);
    }
    if (!needsFetch) {
      console.log('settings saved without weather changes; fetch skipped');
    }
  }
  Pebble.sendAppMessage(msg, afterSave, afterSave);
});
