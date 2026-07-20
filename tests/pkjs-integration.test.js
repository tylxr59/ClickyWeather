'use strict';

var assert = require('assert');
var Module = require('module');
var originalLoad = Module._load;
var listeners = {};
var sent = [];
var keys = { ClockIs24h: 1, LastUpdated: 2, RequestSeq: 3 };

function FakeClay() {}
FakeClay.prototype.generateUrl = function() { return 'https://example.invalid'; };

Module._load = function(request, parent, isMain) {
  if (request === '@rebble/clay') return FakeClay;
  if (request === 'message_keys') return keys;
  return originalLoad.call(this, request, parent, isMain);
};

var storage = {};
global.localStorage = {
  getItem: function(key) {
    return storage.hasOwnProperty(key) ? storage[key] : null;
  },
  setItem: function(key, value) { storage[key] = String(value); },
  removeItem: function(key) { delete storage[key]; }
};

global.Pebble = {
  addEventListener: function(name, callback) { listeners[name] = callback; },
  openURL: function() {},
  sendAppMessage: function(message, success) {
    sent.push(message);
    if (success) success();
  }
};

var geoMode = 'error';
Object.defineProperty(global, 'navigator', {
  configurable: true,
  value: {
    geolocation: {
      getCurrentPosition: function(success, failure) {
        if (geoMode === 'success') {
          success({ coords: { latitude: 40, longitude: -75 } });
        } else {
          failure({ message: 'permission denied' });
        }
      }
    }
  }
});

function forecastFixture() {
  var hourlyTimes = [];
  for (var hour = 10; hour <= 17; hour++) {
    hourlyTimes.push('2026-07-19T' + hour + ':00');
  }
  return {
    utc_offset_seconds: -14400,
    current: {
      time: '2026-07-19T10:00',
      temperature_2m: 70,
      apparent_temperature: 71,
      relative_humidity_2m: 50,
      dew_point_2m: 51,
      weather_code: 0,
      wind_speed_10m: 5,
      wind_direction_10m: 90,
      wind_gusts_10m: 8,
      uv_index: 4
    },
    hourly: {
      time: hourlyTimes,
      temperature_2m: [70, 71, 72, 73, 74, 75, 76, 77],
      weather_code: [0, 0, 1, 2, 3, 3, 2, 1],
      precipitation_probability: [0, 10, 20, 30, 40, 50, 60, 70],
      wind_speed_10m: [5, 5, 6, 6, 7, 7, 8, 8],
      wind_direction_10m: [90, 90, 90, 90, 90, 90, 90, 90],
      precipitation: [0, 25.4, 0, 0, 0, 0, 0, 0],
      uv_index: [4, 5, 6, 5, 4, 3, 2, 1]
    },
    daily: {
      time: ['2026-07-19', '2026-07-20', '2026-07-21', '2026-07-22', '2026-07-23'],
      weather_code: [0, 1, 2, 3, 0],
      temperature_2m_max: [75, 76, 77, 78, 79],
      temperature_2m_min: [60, 61, 62, 63, 64],
      precipitation_probability_max: [0, 10, 20, 30, 40],
      sunrise: ['2026-07-19T05:45'],
      sunset: ['2026-07-19T20:15'],
      uv_index_max: [6]
    }
  };
}

global.XMLHttpRequest = function() {
  this.status = 0;
  this.responseText = '';
  this.open = function(_method, url) { this.url = url; };
  this.abort = function() {};
  this.send = function() {
    if (this.url.indexOf('api.github.com') >= 0) {
      this.status = 200;
      this.responseText = JSON.stringify({ tag_name: 'v1.4.0' });
      this.onload();
    } else if (this.url.indexOf('api.open-meteo.com/v1/forecast') >= 0) {
      this.status = 200;
      this.responseText = JSON.stringify(forecastFixture());
      this.onload();
    } else {
      this.onerror();
    }
  };
};

storage['clay-settings'] = JSON.stringify({
  Units: { value: '1' },
  UseDewPoint: { value: true },
  TimeFormat: { value: '2' },
  Theme: { value: '1' },
  LoopNavigation: { value: false },
  AnimationsEnabled: { value: false },
  BackgroundUpdateInterval: { value: '3600' },
  AppUpdateCheckInterval: { value: '86400' },
  Toggle0: { value: true },
  Toggle1: { value: false }
});

require('../src/pkjs/index');
Module._load = originalLoad;

listeners.ready();
assert.strictEqual(storage.units, 'metric');
assert.strictEqual(storage.useDewPoint, '1');
assert.strictEqual(storage.timeFormat, '2');
assert.strictEqual(sent[0].Theme, 1);
assert.strictEqual(sent[0].BackgroundUpdateInterval, 3600);
assert.strictEqual(sent[0].EnabledMask, 509);

sent.length = 0;
listeners.appmessage({ payload: { '2': 1, '3': 7 } });
var locationError = sent.filter(function(message) {
  return message.ResponseSeq === 7;
})[0];
assert.strictEqual(locationError.FetchError, 2);

sent.length = 0;
storage.units = 'imperial';
geoMode = 'success';
listeners.appmessage({ payload: { '1': 0, '2': 1, '3': 8 } });
var weather = sent.filter(function(message) {
  return message.ResponseSeq === 8 && message.Temp === 70;
})[0];
assert.ok(weather);
assert.strictEqual(weather.FetchError, 0);
assert.strictEqual(weather.Hour1Precip, 10);
assert.strictEqual(weather.AQI, -1);
assert.strictEqual(weather.PollenLevel, -1);
assert.strictEqual(weather.AlertCategory, -1);

console.log('pkjs integration tests passed');
