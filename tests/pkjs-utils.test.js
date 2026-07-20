'use strict';

var assert = require('assert');
var utils = require('../src/pkjs/weather-utils');

assert.strictEqual(utils.mapWeatherCode(0), utils.COND.SUNNY);
assert.strictEqual(utils.mapWeatherCode(63), utils.COND.RAIN);
assert.strictEqual(utils.mapWeatherCode(96), utils.COND.STORM);

assert.deepStrictEqual(utils.parseCoordinateOverride('37.7,-122.4'), {
  lat: 37.7,
  lon: -122.4
});
assert.strictEqual(utils.parseCoordinateOverride('not coordinates'), null);
assert.strictEqual(utils.parseCoordinateOverride('37.7x,-122.4'), null);
assert.strictEqual(utils.validCoordinates(91, 0), false);

assert.strictEqual(utils.precipMmToDisplayX10(25.4, 'imperial'), 10);
assert.strictEqual(utils.precipMmToDisplayX10(5, 'imperial'), 2);
assert.strictEqual(utils.precipMmToDisplayX10(0.1, 'metric'), 1);

assert.strictEqual(utils.isNewerVersion('v1.4.0', '1.3.1'), true);
assert.strictEqual(utils.isNewerVersion('v1.3.1', '1.3.1'), false);
assert.strictEqual(utils.isNewerVersion('invalid', '1.3.1'), false);

var alertFailure = utils.classifyNwsResponse(new Error('offline'), null);
assert.strictEqual(alertFailure.active, 0);
assert.strictEqual(alertFailure.category, utils.ALERT_CAT.UNKNOWN);
assert.strictEqual(utils.classifyNwsResponse(null, {}).category,
  utils.ALERT_CAT.UNKNOWN);
var allClear = utils.classifyNwsResponse(null, { features: [] });
assert.strictEqual(allClear.category, utils.ALERT_CAT.NONE);
var tornado = utils.classifyNwsResponse(null, {
  features: [{ properties: { event: 'Tornado Warning' } }]
});
assert.strictEqual(tornado.active, 1);
assert.strictEqual(tornado.category, utils.ALERT_CAT.TORNADO);

var missingAir = utils.airQualityFields(null, -1);
assert.deepStrictEqual(missingAir, {
  AQI: -1,
  PM25: -1,
  PM10: -1,
  O3: -1,
  NO2: -1,
  PollenLevel: -1
});
var freshAir = utils.airQualityFields({ current: {
  us_aqi: 42.2,
  pm2_5: 8.7,
  pm10: null,
  ozone: 50.1,
  nitrogen_dioxide: 11.8
}}, 3);
assert.strictEqual(freshAir.AQI, 42);
assert.strictEqual(freshAir.PM25, 9);
assert.strictEqual(freshAir.PM10, -1);
assert.strictEqual(freshAir.PollenLevel, 3);

var numericPayload = { '17': 1 };
assert.strictEqual(utils.payloadValue(numericPayload, 'RequestSeq', 0,
  { RequestSeq: 17 }), 1);
assert.strictEqual(utils.payloadValue({ RequestSeq: 2 }, 'RequestSeq', 0,
  { RequestSeq: 17 }), 2);

console.log('pkjs utility tests passed');
