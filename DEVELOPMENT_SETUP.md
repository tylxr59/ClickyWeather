# Development Setup

This document describes how to configure the project for local development after cloning or after
cleaning secrets for a GitHub commit.

---

## Radar Proxy Authentication

The Vercel radar proxy (`proxy/api/radar.js`) supports an optional shared-secret key. When the
`RADAR_SECRET` environment variable is set on the Vercel project, all requests must include a
matching `?key=` query parameter or the server returns `401 Unauthorized`.

The secret is **never stored in source control**. It lives in two places only:

| Location | What to store |
|---|---|
| Vercel Project → Settings → Environment Variables | The raw (unencoded) secret value |
| `src/pkjs/index.js` — `RADAR_PROXY_URL` variable | The URL with the secret URL-encoded in the `?key=` param |

---

## Restoring the Key After a Clean Commit

When the repo is cleaned for GitHub, `RADAR_PROXY_URL` in `src/pkjs/index.js` is set to the bare
proxy URL with no key:

```javascript
var RADAR_PROXY_URL = 'https://touchyweather-radar-proxy.vercel.app/api/radar';
```

To restore the key for local development or building:

### Step 1 — Retrieve the secret

The raw secret value is stored only in the Vercel dashboard:

> Vercel → `touchyweather-radar-proxy` project → Settings → Environment Variables → `RADAR_SECRET`

### Step 2 — URL-encode the secret

Certain characters in the secret must be percent-encoded for use in a URL query string.
Common substitutions:

| Character | Encoded form |
|---|---|
| `@` | `%40` |
| `#` | `%23` |
| `&` | `%26` |
| `+` | `%2B` |
| `=` | `%3D` |
| ` ` (space) | `%20` |

You can also URL-encode the entire value in a browser console or Node.js:

```javascript
encodeURIComponent('your-raw-secret-here')
```

### Step 3 — Update `RADAR_PROXY_URL`

Open `src/pkjs/index.js` and find the `RADAR_PROXY_URL` variable (around line 372).
Replace the bare URL with the authenticated form:

```javascript
var RADAR_PROXY_URL = 'https://touchyweather-radar-proxy.vercel.app/api/radar?key=<URL-ENCODED-SECRET>';
```

Substitute `<URL-ENCODED-SECRET>` with the encoded value from Step 2.

### Step 4 — Build

```bash
pebble build
```

The webpack step bundles `src/pkjs/index.js` into `build/pebble-js-app.js`, which is what the
watch installs. The key is embedded in the build output.

---

## Before Committing to GitHub

**Always** revert `RADAR_PROXY_URL` to the bare URL before pushing:

```javascript
var RADAR_PROXY_URL = 'https://touchyweather-radar-proxy.vercel.app/api/radar';
```

The `build/` directory is gitignored, so the compiled output (which also contains the key) will
not be committed.

---

## Vercel Environment Variable Reference

| Variable | Required | Description |
|---|---|---|
| `RADAR_SECRET` | No | Shared secret for radar proxy auth. If absent, all requests are allowed (open). |

To add or update the secret in Vercel:

1. Go to the Vercel dashboard → `touchyweather-radar-proxy` project
2. Settings → Environment Variables
3. Add/update `RADAR_SECRET` with the raw (unencoded) value
4. Set target environment to **Production**
5. **Redeploy** — env vars only take effect after a new deployment

---

## Testing Auth on the Live Endpoint

After redeploying, verify with:

```bash
# Should return 401
curl -s -w "\nHTTP:%{http_code}" "https://touchyweather-radar-proxy.vercel.app/api/radar?lat=37.77&lon=-122.41"

# Should return 401
curl -s -w "\nHTTP:%{http_code}" "https://touchyweather-radar-proxy.vercel.app/api/radar?lat=37.77&lon=-122.41&key=wrongkey"

# Should return 200
curl -s -o /dev/null -w "HTTP:%{http_code}" "https://touchyweather-radar-proxy.vercel.app/api/radar?lat=37.77&lon=-122.41&key=<URL-ENCODED-SECRET>"
```
