'use strict';

var COND = {
  SUNNY: 0, PARTLY_CLOUDY: 1, CLOUDY: 2, RAIN: 3, SNOW: 4, STORM: 5, FOG: 6
};

var ALERT_CAT = {
  NONE: 0,
  WIND: 1,
  HEAT: 2,
  COLD: 3,
  FLOOD: 4,
  TORNADO: 5,
  WINTER: 6,
  OTHER: 7,
  UNKNOWN: -1
};

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
  var dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
  return dirs[Math.round(deg / 45) % 8];
}

function validCoordinates(lat, lon) {
  return isFinite(lat) && isFinite(lon) &&
         lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
}

function parseCoordinateOverride(value) {
  if (!value) return null;
  var parts = value.split(',');
  if (parts.length !== 2) return null;
  if (!parts[0].trim() || !parts[1].trim()) return null;
  var lat = Number(parts[0]);
  var lon = Number(parts[1]);
  if (!validCoordinates(lat, lon)) return null;
  return { lat: lat, lon: lon };
}

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

function classifyNwsResponse(err, data) {
  if (err || !data || !Array.isArray(data.features)) {
    return { active: 0, category: ALERT_CAT.UNKNOWN };
  }
  if (!data.features.length) {
    return { active: 0, category: ALERT_CAT.NONE };
  }
  var properties = data.features[0].properties || {};
  if (!properties.event) {
    return { active: 0, category: ALERT_CAT.UNKNOWN };
  }
  return { active: 1, category: nwsEventToCategory(properties.event) };
}

function parseVersion(value) {
  var match = String(value || '').match(/^v?(\d+)\.(\d+)\.(\d+)(?:[-+].*)?$/);
  if (!match) return null;
  return [parseInt(match[1], 10), parseInt(match[2], 10), parseInt(match[3], 10)];
}

function isNewerVersion(latest, current) {
  var a = parseVersion(latest);
  var b = parseVersion(current);
  if (!a || !b) return false;
  for (var i = 0; i < 3; i++) {
    if (a[i] > b[i]) return true;
    if (a[i] < b[i]) return false;
  }
  return false;
}

function precipMmToDisplayX10(mm, units) {
  var value = typeof mm === 'number' && isFinite(mm) ? mm : 0;
  return Math.round((units === 'metric' ? value : value / 25.4) * 10);
}

function roundedOrUnknown(value) {
  return typeof value === 'number' && isFinite(value) ? Math.round(value) : -1;
}

function airQualityFields(data, pollenLevel) {
  var current = data && data.current;
  var fields = {
    AQI: -1,
    PM25: -1,
    PM10: -1,
    O3: -1,
    NO2: -1,
    PollenLevel: typeof pollenLevel === 'number' ? pollenLevel : -1
  };
  if (!current) return fields;
  fields.AQI = roundedOrUnknown(current.us_aqi);
  fields.PM25 = roundedOrUnknown(current.pm2_5);
  fields.PM10 = roundedOrUnknown(current.pm10);
  fields.O3 = roundedOrUnknown(current.ozone);
  fields.NO2 = roundedOrUnknown(current.nitrogen_dioxide);
  return fields;
}

function payloadValue(payload, name, fallback, keys) {
  if (payload[name] !== undefined) return payload[name];
  var key = keys && keys[name];
  if (key !== undefined && payload[key] !== undefined) return payload[key];
  if (key !== undefined && payload[String(key)] !== undefined) {
    return payload[String(key)];
  }
  return fallback;
}

module.exports = {
  ALERT_CAT: ALERT_CAT,
  COND: COND,
  airQualityFields: airQualityFields,
  classifyNwsResponse: classifyNwsResponse,
  degToCompass: degToCompass,
  isNewerVersion: isNewerVersion,
  mapWeatherCode: mapWeatherCode,
  parseCoordinateOverride: parseCoordinateOverride,
  parseVersion: parseVersion,
  payloadValue: payloadValue,
  precipMmToDisplayX10: precipMmToDisplayX10,
  validCoordinates: validCoordinates
};
