# Development

## Build

```bash
npm ci
npm test
npm run build
```

The webpack step bundles `src/pkjs/index.js` into `build/pebble-js-app.js`. The `build/` directory
is gitignored, so compiled output will not be committed.

Install the build in the Time 2 emulator with:

```bash
npm run install:emery
```

Push a `v<package version>` tag after local verification. The tag-triggered GitHub Actions workflow checks that the tag matches the changelog and every package/PKJS version source, runs the tests, builds the PBW, and attaches it to the matching GitHub Release.

Before tagging, add the release at the top of `CHANGELOG.md`, keep its version in
sync with `package.json`, `package-lock.json`, and `src/pkjs/version.js`, then run:

```bash
npm run release:notes -- 1.4.0
node -c src/pkjs/index.js
node -c src/pkjs/config.js
npm test
pebble clean
npm run build
```

The changelog bullets are reused for the one-time on-watch What's New screen and
the GitHub Release body. New code always requires a new version and tag.
