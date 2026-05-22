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
