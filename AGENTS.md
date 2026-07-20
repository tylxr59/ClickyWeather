# ClickyWeather contributor guidance

## Privacy is a release requirement

User privacy is paramount. Never add analytics, telemetry, tracking pixels,
usage counters, advertising identifiers, fingerprinting, or background calls
whose purpose is to measure users. Do not port such code from TouchyWeather or
another upstream. Weather, alert, update-check, and explicitly user-requested
configuration traffic must be limited to the data necessary for that feature
and documented in the README. Any proposal that would transmit additional user
or device information requires explicit maintainer approval and a privacy review.

## Release notes are required

Every release must add a newest-first `## x.y.z` entry to `CHANGELOG.md` with
plain-language `- ` bullets describing user-visible changes. Keep that version
identical in `package.json`, `package-lock.json`, and `src/pkjs/version.js`.
Never reuse an existing version or tag for new code.

The build generates the one-time on-watch What's New screen from the newest
changelog entry. The release workflow publishes the same entry as the GitHub
Release body. Before tagging, run `npm run release:notes -- <version>`,
`node -c src/pkjs/index.js`, `node -c src/pkjs/config.js`, and a clean Pebble
build. Publish with a matching `v<version>` tag; manual workflow runs may only
repair the PBW attached to an existing tag.

## Product boundaries

Keep ClickyWeather button-first. UP/DOWN navigate, SELECT is the primary action,
and BACK exits unless a real nested view is open. Do not restore upstream radar,
analytics, on-watch settings, or Big Mode without explicit maintainer direction.
Preserve the supported `emery` and `gabbro` targets unless scope changes.
