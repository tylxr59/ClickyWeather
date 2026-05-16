module.exports = [
  {
    "type": "heading",
    "defaultValue": "TouchyWeather Settings"
  },
  {
    "type": "text",
    "defaultValue": "Configure your weather app. Theme and units sync to the watch immediately."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance"
      },
      {
        "type": "radiogroup",
        "messageKey": "Theme",
        "label": "Theme",
        "defaultValue": "0",
        "options": [
          { "label": "Light", "value": "0" },
          { "label": "Dark", "value": "1" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Units"
      },
      {
        "type": "radiogroup",
        "messageKey": "Units",
        "label": "Measurement system",
        "defaultValue": "0",
        "options": [
          { "label": "Imperial (°F, mph)", "value": "0" },
          { "label": "Metric (°C, km/h)",  "value": "1" }
        ]
      },
      {
        "type": "toggle",
        "messageKey": "UseDewPoint",
        "label": "Show dew point",
        "description": "Replace the humidity reading on the main card with dew point temperature.",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Location"
      },
      {
        "type": "input",
        "messageKey": "LocationOverride",
        "label": "Manual override",
        "description": "Optional. Format: lat,lon (e.g. 37.7749,-122.4194). Leave empty to use phone GPS.",
        "attributes": {
          "placeholder": "lat,lon",
          "limit": 32
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
