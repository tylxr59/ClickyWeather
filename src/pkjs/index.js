// ClickyWeather PKJS - Open-Meteo fetcher
// No API key required. Fetches forecast + air-quality and posts AppMessage.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var COND = {
  SUNNY: 0, PARTLY_CLOUDY: 1, CLOUDY: 2, RAIN: 3, SNOW: 4, STORM: 5, FOG: 6
};

// Map Open-Meteo WMO weather codes to our internal enum.
function mapWeatherCode(code) {
  if (code === 0) return COND.SUNNY;
  if (code === 1 || code === 2) return COND.PARTLY_CLOUDY;
  if (code === 3) return COND.CLOUDY;
  if (code >= 45 && code <= 48) return COND.FOG;
  if (code >= 51 && code <= 67) return COND.RAIN;
  if (code >= 71 && code <= 77) return COND.SNOW;
  if (code >= 80 && code <= 82) return COND.RAIN;
  if (code >= 85 && code <= 86) return COND.SNOW;
  if (code >= 95 && code <= 99) return COND.STORM;
  return COND.PARTLY_CLOUDY;
}

function degToCompass(deg) {
  var dirs = ['N','NE','E','SE','S','SW','W','NW'];
  return dirs[Math.round(deg / 45) % 8];
}

function fmtTime12(iso) {
  if (!iso) return '';
  var t = iso.split('T')[1] || '';
  var parts = t.split(':');
  var h = parseInt(parts[0], 10);
  var m = parts[1] || '00';
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

// Returns true when coordinates are within the contiguous US + Alaska
// bounding box. NWS alerts are only available for US points.
function isUS(lat, lon) {
  return lat >= 24 && lat <= 50 && lon >= -125 && lon <= -66;
}

// Alert category enum values — must match AlertCategory in weather_data.h.
var ALERT_CAT = {
  NONE:    0,
  WIND:    1,
  HEAT:    2,
  COLD:    3,
  FLOOD:   4,
  TORNADO: 5,
  WINTER:  6,
  OTHER:   7,
  UNKNOWN: -1
};

// Map an NWS event string to an alert category integer.
// Matching is case-insensitive substring check. Order matters: more-
// specific patterns (e.g. "wind chill") must precede generic ones ("wind").
function nwsEventToCategory(event) {
  var e = (event || '').toLowerCase();
  if (e.indexOf('tornado') >= 0 || e.indexOf('hurricane') >= 0 ||
      e.indexOf('typhoon') >= 0) return ALERT_CAT.TORNADO;
  if (e.indexOf('cold') >= 0 || e.indexOf('wind chill') >= 0 ||
      e.indexOf('freeze') >= 0 || e.indexOf('frost') >= 0) return ALERT_CAT.COLD;
  if (e.indexOf('wind') >= 0) return ALERT_CAT.WIND;
  if (e.indexOf('heat') >= 0) return ALERT_CAT.HEAT;
  if (e.indexOf('flood') >= 0) return ALERT_CAT.FLOOD;
  if (e.indexOf('winter') >= 0 || e.indexOf('blizzard') >= 0 ||
      e.indexOf('ice storm') >= 0 || e.indexOf('snow') >= 0) return ALERT_CAT.WINTER;
  return ALERT_CAT.OTHER;
}

// Returns true when coordinates are within continental Europe + UK +
// Scandinavia. Open-Meteo CAMS pollen data is reliable inside this box;
// outside it we fall back to the Google Pollen proxy.
function isEurope(lat, lon) {
  return lat >= 35 && lat <= 72 && lon >= -25 && lon <= 45;
}

// Convert Open-Meteo pollen values (grains/m³) to the 0-5 UPI-style
// scale used by the Google Pollen API, using European Aeroallergen
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
    weedScale(ragweed), weedScale(mugwort), grassScale(olive),
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
    var ampm = h >= 12 ? 'PM' : 'AM';
    h = h % 12; if (h === 0) h = 12;
    return h + ':' + (m < 10 ? '0' + m : m) + ' ' + ampm;
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
  req.open('GET', url, true);
  req.timeout = 15000;
  req.onload = function() {
    if (req.status >= 200 && req.status < 300) {
      try { cb(null, JSON.parse(req.responseText)); }
      catch (e) { cb(e); }
    } else {
      cb(new Error('HTTP ' + req.status));
    }
  };
  req.onerror = function() { cb(new Error('xhr error')); };
  req.ontimeout = function() { cb(new Error('xhr timeout')); };
  req.send();
}

function fetchWeather(lat, lon) {
  var units = getUnits();
  var tempUnit = units === 'metric' ? 'celsius' : 'fahrenheit';
  var windUnit = units === 'metric' ? 'kmh' : 'mph';

  var fc = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat + '&longitude=' + lon +
    '&current=temperature_2m,apparent_temperature,relative_humidity_2m,dew_point_2m,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m' +
    '&hourly=temperature_2m,weather_code,precipitation_probability,precipitation' +
    '&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,uv_index_max' +
    '&temperature_unit=' + tempUnit +
    '&wind_speed_unit=' + windUnit +
    '&timezone=auto&forecast_days=4';

  var aq = 'https://air-quality-api.open-meteo.com/v1/air-quality' +
    '?latitude=' + lat + '&longitude=' + lon +
    // Always request pollen fields. CAMS covers Europe; outside that
    // region the fields return null and we fall back to Google.
    '&current=us_aqi,grass_pollen,birch_pollen,alder_pollen,ragweed_pollen,mugwort_pollen,olive_pollen' +
    '&timezone=auto';

  xhr(fc, function(err, data) {
    if (err) { console.log('forecast err: ' + err.message); return; }
    xhr(aq, function(_e2, aqd) {
      var msg = {};
      try {
        var cur = data.current || {};
        var daily = data.daily || {};
        var hourly = data.hourly || {};
        msg.Temp = Math.round(cur.temperature_2m);
        msg.FeelsLike = Math.round(cur.apparent_temperature);
        msg.Humidity = Math.round(cur.relative_humidity_2m);
        msg.DewPoint = Math.round(cur.dew_point_2m);
        msg.UseDewPoint = getUseDewPoint() ? 1 : 0;
        msg.Wind = Math.round(cur.wind_speed_10m);
        msg.WindDir = degToCompass(cur.wind_direction_10m || 0);
        msg.WindGust = Math.round(cur.wind_gusts_10m || 0);
        msg.Condition = mapWeatherCode(cur.weather_code);
        if (daily.temperature_2m_max && daily.temperature_2m_max.length) {
          msg.High = Math.round(daily.temperature_2m_max[0]);
          msg.Low = Math.round(daily.temperature_2m_min[0]);
        }
        if (daily.sunrise && daily.sunrise.length) {
          msg.Sunrise = fmtTime12(daily.sunrise[0]);
          msg.Sunset = fmtTime12(daily.sunset[0]);
        }
        if (daily.uv_index_max && daily.uv_index_max.length) {
          msg.UV = Math.round(daily.uv_index_max[0]);
        }
        var p = hourly.precipitation_probability || [];
        var times = hourly.time || [];
        // Find the index of the current hour in the hourly arrays so the
        // 5 bars truly represent "Now / +1h / +2h / +3h / +4h" instead of
        // starting at midnight. Open-Meteo returns local-tz timestamps
        // like "2026-05-06T14:00" when timezone=auto.
        var startIdx = 0;
        if (times.length) {
          var now = new Date();
          var pad = function(n) { return n < 10 ? '0' + n : '' + n; };
          var nowKey = now.getFullYear() + '-' + pad(now.getMonth() + 1) +
                       '-' + pad(now.getDate()) + 'T' + pad(now.getHours()) + ':00';
          for (var k = 0; k < times.length; k++) {
            if (times[k] === nowKey) { startIdx = k; break; }
          }
        }
        for (var i = 0; i < 5; i++) {
          msg['Precip' + i] = Math.round(p[startIdx + i] || 0);
        }
        var alert = -1;
        for (var j = 0; j < 5; j++) {
          var pj = p[startIdx + j];
          if (pj !== undefined && pj >= 50) {
            alert = j === 0 ? 15 : j * 60;
            break;
          }
        }
        msg.RainAlertMinutes = alert;
        // Precipitation amount for the current hour (mm×10 integer, always metric
        // from Open-Meteo regardless of wind_speed_unit). C converts to
        // tenths-of-inch for imperial display at render time.
        var precipMm = (hourly.precipitation || [])[startIdx] || 0;
        msg.PrecipAmount = Math.round(precipMm * 10);
        if (aqd && aqd.current) {
          msg.AQI = Math.round(aqd.current.us_aqi || 0);
        }
        msg.Units = units === 'metric' ? 1 : 0;
        msg.LastUpdated = Math.floor(Date.now() / 1000);

        // Pollen — European CAMS strategy:
        //   Europe  → use Open-Meteo CAMS fields already in the AQ
        //             response (free, no quota, zero extra requests).
        //   Elsewhere → PollenLevel is not set (watch shows no pollen data).
        // Phase 10A: Next 6 Hours (offsets +1h..+6h from current hour).
        var temps = hourly.temperature_2m || [];
        var codes = hourly.weather_code || [];
        for (var hi = 1; hi <= 6; hi++) {
          var idx = startIdx + hi;
          var hourLabel = '';
          if (times[idx]) {
            var hh = parseInt(times[idx].split('T')[1].split(':')[0], 10);
            var ampm = hh >= 12 ? 'PM' : 'AM';
            hh = hh % 12; if (hh === 0) hh = 12;
            hourLabel = hh + ' ' + ampm;
          }
          msg['Hour' + hi + 'Label'] = hourLabel;
          msg['Hour' + hi + 'Temp']  = Math.round(temps[idx] || 0);
          msg['Hour' + hi + 'Cond']  = mapWeatherCode(codes[idx] || 0);
          msg['Hour' + hi + 'Pop']   = Math.round(p[idx] || 0);
        }

        // Phase 10B: Week Ahead (today + next 3 days = 4 total).
        var dayCodes = daily.weather_code || [];
        var dayHigh  = daily.temperature_2m_max || [];
        var dayLow   = daily.temperature_2m_min || [];
        var dayPop   = daily.precipitation_probability_max || [];
        var dayTimes = daily.time || [];
        var dayNames = ['SUN','MON','TUE','WED','THU','FRI','SAT'];
        for (var di = 0; di < 4; di++) {
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

        // Attempt Open-Meteo pollen (Europe only). If successful, skip
        // the Google proxy call entirely for this request.
        if (isEurope(lat, lon) && aqd && aqd.current) {
          var aqc = aqd.current;
          var euUpi = pollenGrainsToUpi(
            aqc.grass_pollen,  aqc.birch_pollen,  aqc.alder_pollen,
            aqc.ragweed_pollen, aqc.mugwort_pollen, aqc.olive_pollen
          );
          if (euUpi >= 0) {
            msg.PollenLevel = euUpi;
          }
        }

        // Weather alerts — three cases:
        //   US  → NWS free public API (chained XHR below)
        //   EU  → synthetic: WMO daily code ≥ 95 = severe thunderstorm
        //   Else→ ALERT_CAT_UNKNOWN (-1), shown as "NO DATA"
        if (isEurope(lat, lon)) {
          var todayCode = (daily.weather_code || [])[0] || 0;
          if (todayCode >= 95) {
            msg.AlertActive   = 1;
            msg.AlertCategory = ALERT_CAT.TORNADO; // closest severe category
          } else {
            msg.AlertActive   = 0;
            msg.AlertCategory = ALERT_CAT.NONE;
          }
          // EU path: send immediately (no extra XHR needed).
          Pebble.sendAppMessage(msg,
            function() { console.log('weather sent'); },
            function(e) { console.log('send fail: ' + JSON.stringify(e)); }
          );
        } else if (isUS(lat, lon)) {
          // US path: fetch NWS alerts then send.
          var nwsUrl = 'https://api.weather.gov/alerts/active?point=' +
                       lat + '%2C' + lon +
                       '&status=actual&message_type=alert';
          xhr(nwsUrl, function(_enws, nwsd) {
            try {
              var features = nwsd && nwsd.features;
              if (features && features.length > 0) {
                var event = features[0].properties && features[0].properties.event;
                msg.AlertActive   = 1;
                msg.AlertCategory = nwsEventToCategory(event);
              } else {
                msg.AlertActive   = 0;
                msg.AlertCategory = ALERT_CAT.NONE;
              }
            } catch (enws2) {
              msg.AlertActive   = 0;
              msg.AlertCategory = ALERT_CAT.NONE;
            }
            Pebble.sendAppMessage(msg,
              function() { console.log('weather sent'); },
              function(e) { console.log('send fail: ' + JSON.stringify(e)); }
            );
          });
        } else {
          // Elsewhere: no alert data available.
          msg.AlertActive   = 0;
          msg.AlertCategory = ALERT_CAT.UNKNOWN;
          Pebble.sendAppMessage(msg,
            function() { console.log('weather sent'); },
            function(e) { console.log('send fail: ' + JSON.stringify(e)); }
          );
        }
      } catch (e) {
        console.log('parse err: ' + e.message);
        return;
      }
    });
  });
}

function locateAndFetch() {
  var override = localStorage.getItem('locationOverride');
  if (override) {
    var parts = override.split(',');
    if (parts.length === 2) {
      fetchWeather(parseFloat(parts[0]), parseFloat(parts[1]));
      return;
    }
  }
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      fetchWeather(pos.coords.latitude, pos.coords.longitude);
    },
    function(err) {
      console.log('geo err: ' + err.message + ' — using fallback');
      fetchWeather(37.7749, -122.4194);
    },
    { timeout: 15000, maximumAge: 600000 }
  );
}

Pebble.addEventListener('ready', function() {
  console.log('ClickyWeather PKJS ready');
  locateAndFetch();
});

Pebble.addEventListener('appmessage', function() {
  locateAndFetch();
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;
  var dict = clay.getSettings(e.response, false);
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
  if (dict.LocationOverride !== undefined && dict.LocationOverride.value) {
    localStorage.setItem('locationOverride', dict.LocationOverride.value);
  } else {
    localStorage.removeItem('locationOverride');
  }
  
  // Handle card toggle settings and build EnabledMask.
  // Each bit in EnabledMask represents a card's enabled state (bit 0 = Toggle0, etc).
  var enabledMask = 0;
  for (var i = 0; i < 9; i++) {
    var key = 'Toggle' + i;
    if (dict[key] !== undefined && dict[key].value) {
      enabledMask |= (1 << i);
    }
    // Store in localStorage for persistence and reference
    localStorage.setItem('toggle' + i, dict[key] && dict[key].value ? '1' : '0');
  }
  
  var msg = clay.getSettings(e.response);
  msg.EnabledMask = enabledMask;
  Pebble.sendAppMessage(msg, function() {
    locateAndFetch();
  }, function() {
    locateAndFetch();
  });
});
