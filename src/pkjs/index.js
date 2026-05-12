// TouchyWeather PKJS - Open-Meteo fetcher
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

// Last-known lat/lon (cached so the Radar card can re-fetch without
// re-running geolocation, and so a Radar request that arrives before
// any weather fetch still has somewhere to look).
var lastLat = null;
var lastLon = null;

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
  lastLat = lat;
  lastLon = lon;
  var units = getUnits();
  var tempUnit = units === 'metric' ? 'celsius' : 'fahrenheit';
  var windUnit = units === 'metric' ? 'kmh' : 'mph';

  var fc = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat + '&longitude=' + lon +
    '&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m' +
    '&hourly=temperature_2m,weather_code,precipitation_probability' +
    '&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,uv_index_max' +
    '&temperature_unit=' + tempUnit +
    '&wind_speed_unit=' + windUnit +
    '&timezone=auto&forecast_days=4';

  var aq = 'https://air-quality-api.open-meteo.com/v1/air-quality' +
    '?latitude=' + lat + '&longitude=' + lon +
    '&current=us_aqi&timezone=auto';

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
        msg.Wind = Math.round(cur.wind_speed_10m);
        msg.WindDir = degToCompass(cur.wind_direction_10m || 0);
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
        if (aqd && aqd.current) {
          msg.AQI = Math.round(aqd.current.us_aqi || 0);
        }
        msg.Units = units === 'metric' ? 1 : 0;
        msg.LastUpdated = Math.floor(Date.now() / 1000);

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
      } catch (e) {
        console.log('parse err: ' + e.message);
        return;
      }
      Pebble.sendAppMessage(msg,
        function() { console.log('weather sent'); },
        function(e) { console.log('send fail: ' + JSON.stringify(e)); }
      );
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

// ----------------------------------------------------------------------
// Phase 12: Radar streaming.
// ----------------------------------------------------------------------

var RADAR_CHUNK_SIZE = 1500;
var RADAR_PROXY_URL = 'https://touchyweather-radar-proxy.vercel.app/api/radar';

// Hand-rolled base64 decoder for PKJS runtimes lacking `atob`.
function decodeBase64(s) {
  var chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  var lookup = {};
  for (var i = 0; i < chars.length; i++) lookup[chars.charAt(i)] = i;
  var out = '';
  s = String(s).replace(/[^A-Za-z0-9+/]/g, '');
  for (var j = 0; j < s.length; j += 4) {
    var n = (lookup[s.charAt(j)] << 18) |
            (lookup[s.charAt(j + 1)] << 12) |
            ((lookup[s.charAt(j + 2)] || 0) << 6) |
            (lookup[s.charAt(j + 3)] || 0);
    out += String.fromCharCode((n >> 16) & 0xff);
    if (s.charAt(j + 2) !== '=' && s.charAt(j + 2) !== '')
      out += String.fromCharCode((n >> 8) & 0xff);
    if (s.charAt(j + 3) !== '=' && s.charAt(j + 3) !== '')
      out += String.fromCharCode(n & 0xff);
  }
  return out;
}

function getRadarProxyURL() {
  return RADAR_PROXY_URL;
}

function sendRadarStatus(status) {
  Pebble.sendAppMessage({ RadarStatus: status });
}

function sendRadarChunk(chunkIdx, total, w, h, ts, byteArr, onAllDone) {
  function send() {
    var msg = {
      RadarChunkIdx: chunkIdx,
      RadarChunkTotal: total,
      RadarChunkData: byteArr,
      RadarWidth: w,
      RadarHeight: h,
      RadarTimestamp: ts,
    };
    Pebble.sendAppMessage(msg, function() {
      if (chunkIdx + 1 >= total) {
        if (onAllDone) onAllDone();
      }
    }, function(e) {
      // Retry once after a brief pause; transient drop is common when
      // the watch is busy redrawing.
      console.log('radar chunk ' + chunkIdx + ' failed, retrying');
      setTimeout(send, 1500);
    });
  }
  send();
}

function fetchRadar() {
  var proxy = getRadarProxyURL();
  if (!proxy) {
    console.log('radar: no RadarProxyURL configured');
    sendRadarStatus(3);
    return;
  }
  if (lastLat === null || lastLon === null) {
    // Trigger geolocation, then radar.
    navigator.geolocation.getCurrentPosition(function(pos) {
      lastLat = pos.coords.latitude;
      lastLon = pos.coords.longitude;
      fetchRadar();
    }, function() {
      sendRadarStatus(3);
    }, { timeout: 15000, maximumAge: 600000 });
    return;
  }

  var url = proxy + (proxy.indexOf('?') >= 0 ? '&' : '?') +
            'lat=' + lastLat + '&lon=' + lastLon + '&format=base64';
  console.log('radar: fetching ' + url);

  var x = new XMLHttpRequest();
  x.open('GET', url, true);
  x.timeout = 25000;
  x.onload = function() {
    if (x.status !== 200) {
      console.log('radar: proxy HTTP ' + x.status);
      sendRadarStatus(3);
      return;
    }
    var ts = parseInt(x.getResponseHeader('X-Radar-Time') || '0', 10) || 0;
    // Decode base64 → byte array. PKJS may lack atob; fall back to a
    // hand-rolled decoder.
    var b64 = (x.responseText || '').trim();
    var bin;
    try {
      bin = (typeof atob === 'function')
        ? atob(b64)
        : decodeBase64(b64);
    } catch (err) {
      console.log('radar: base64 decode err ' + err.message);
      sendRadarStatus(3);
      return;
    }
    var total_bytes = bin.length;
    var w = 160, h = 160;
    if (total_bytes !== w * h) {
      console.log('radar: unexpected payload size ' + total_bytes);
    }
    var totalChunks = Math.ceil(total_bytes / RADAR_CHUNK_SIZE);
    console.log('radar: ' + total_bytes + 'B -> ' + totalChunks + ' chunks');

    function sendChunk(i) {
      if (i >= totalChunks) return;
      var start = i * RADAR_CHUNK_SIZE;
      var end = Math.min(start + RADAR_CHUNK_SIZE, total_bytes);
      var slice = new Array(end - start);
      for (var k = 0; k < slice.length; k++) slice[k] = bin.charCodeAt(start + k);
      sendRadarChunk(i, totalChunks, w, h, ts, slice, function() {
        // all done
      });
      setTimeout(function() { sendChunk(i + 1); }, 80);
    }
    sendChunk(0);
  };
  x.onerror = function() {
    console.log('radar: xhr error');
    sendRadarStatus(3);
  };
  x.ontimeout = function() {
    console.log('radar: xhr timeout');
    sendRadarStatus(3);
  };
  x.send();
}

Pebble.addEventListener('ready', function() {
  console.log('TouchyWeather PKJS ready');
  locateAndFetch();
});

Pebble.addEventListener('appmessage', function(e) {
  var p = (e && e.payload) || {};
  if (p.RadarRequest) {
    fetchRadar();
    return;
  }
  locateAndFetch();
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;
  var dict = clay.getSettings(e.response, false);
  if (dict.Units !== undefined) {
    localStorage.setItem('units', dict.Units.value === 1 ? 'metric' : 'imperial');
  }
  if (dict.LocationOverride !== undefined && dict.LocationOverride.value) {
    localStorage.setItem('locationOverride', dict.LocationOverride.value);
  } else {
    localStorage.removeItem('locationOverride');
  }
  var msg = clay.getSettings(e.response);
  Pebble.sendAppMessage(msg, function() {
    locateAndFetch();
  }, function() {
    locateAndFetch();
  });
});
