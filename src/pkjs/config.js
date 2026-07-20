module.exports = [
  {
    "type": "heading",
    "defaultValue": "ClickyWeather Settings"
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
      },
      {
        "type": "radiogroup",
        "messageKey": "TimeFormat",
        "label": "Time format",
        "defaultValue": "0",
        "options": [
          { "label": "Match watch", "value": "0" },
          { "label": "12-hour (2 PM)", "value": "1" },
          { "label": "24-hour (14:00)", "value": "2" }
        ]
      },
      {
        "type": "toggle",
        "messageKey": "AnimationsEnabled",
        "label": "Animations",
        "description": "Animate briefly after activity, then settle to save battery.",
        "defaultValue": true
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
        "defaultValue": "Navigation"
      },
      {
        "type": "toggle",
        "messageKey": "LoopNavigation",
        "label": "Loop cards at edges",
        "description": "When on, pressing Up on the first card or Down on the last card wraps around the carousel. Turn off to exit the app at the edges.",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Background Updates"
      },
      {
        "type": "select",
        "messageKey": "BackgroundUpdateInterval",
        "label": "Auto-refresh interval",
        "description": "Optionally fetch weather while the app is closed. More frequent updates may reduce battery life.",
        "defaultValue": "0",
        "options": [
          { "label": "Disabled (manual refresh only)", "value": "0" },
          { "label": "Every 30 minutes", "value": "1800" },
          { "label": "Every hour", "value": "3600" },
          { "label": "Every 3 hours", "value": "10800" },
          { "label": "Every 6 hours", "value": "21600" },
          { "label": "Every 12 hours", "value": "43200" },
          { "label": "Every 24 hours", "value": "86400" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "App Updates"
      },
      {
        "type": "select",
        "messageKey": "AppUpdateCheckInterval",
        "label": "Check for new releases",
        "description": "Automatic checks run on the phone when the app opens or weather refreshes; they do not wake the watch on their own. Checking with every weather refresh can increase network use.",
        "defaultValue": "86400",
        "options": [
          { "label": "Never automatically", "value": "0" },
          { "label": "Every weather refresh", "value": "-1" },
          { "label": "At most every 6 hours", "value": "21600" },
          { "label": "At most every 12 hours", "value": "43200" },
          { "label": "At most every day", "value": "86400" },
          { "label": "At most every 3 days", "value": "259200" },
          { "label": "At most every week", "value": "604800" }
        ]
      },
      {
        "type": "toggle",
        "id": "check-app-update-request",
        "messageKey": "CheckForAppUpdate",
        "defaultValue": false
      },
      {
        "type": "button",
        "id": "check-app-update-now",
        "defaultValue": "Check for updates now",
        "description": "Saves and closes settings, then checks GitHub immediately. A newer release will appear on the watch.",
        "primary": false
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
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Cards"
      },
      {
        "type": "text",
        "defaultValue": "Toggle which cards are visible on your watch."
      },
      {
        "type": "toggle",
        "messageKey": "Toggle0",
        "label": "6 Hours",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle1",
        "label": "Week Ahead",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle2",
        "label": "Rain",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle3",
        "label": "UV Index",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle4",
        "label": "Air Quality",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle5",
        "label": "Sun Cycle",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle6",
        "label": "Night Sky",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle7",
        "label": "Golden Hour",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "Toggle8",
        "label": "Alerts",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
