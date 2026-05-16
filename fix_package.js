const fs = require('fs');
let text = fs.readFileSync('package.json', 'utf8');
text = text.replace(/"resources":[\s\S]*\}\s*\}\s*\}/, `"resources": {
    "media": [
      {
        "type": "bitmap",
        "name": "IMAGE_MENU_ICON",
        "file": "images/menu_icon.png"
      }
    ]
  }
}`);
fs.writeFileSync('package.json', text);
