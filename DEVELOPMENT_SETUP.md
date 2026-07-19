# Development

## Build

```bash
npm ci
npm run build
```

The webpack step bundles `src/pkjs/index.js` into `build/pebble-js-app.js`. The `build/` directory
is gitignored, so compiled output will not be committed.

Install the build in the Time 2 emulator with:

```bash
npm run install:emery
```

Push a `v<package version>` tag after local verification. The tag-triggered GitHub Actions workflow checks that the tag matches both version sources, builds the PBW, and attaches it to the matching GitHub Release.
