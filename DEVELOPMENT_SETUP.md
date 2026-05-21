# Development Setup

This document describes how to configure the project for local development after cloning or after
cleaning secrets for a GitHub commit.

---

## Pollen Proxy Authentication

The Vercel pollen proxy (`proxy/api/pollen.js`) supports an optional shared-secret key. When the
`RADAR_SECRET` environment variable is set on the Vercel project, all requests must include a
matching `?key=` query parameter or the server returns `401 Unauthorized`.

> **Note**: The env var is still named `RADAR_SECRET` for historical reasons; it guards both the
> pollen and any other proxy endpoints in the same Vercel project.

The secret is **never stored in source control**. It lives in two places only:

| Location | What to store |
|---|---|
| Vercel Project → Settings → Environment Variables | The raw (unencoded) secret value |
| `proxy/api/pollen.js` (if you add a client-side caller) | The URL with the secret URL-encoded in the `?key=` param |

---

## Vercel Environment Variable Reference

| Variable | Required | Description |
|---|---|---|
| `RADAR_SECRET` | No | Shared secret for proxy auth. If absent, all requests are allowed (open). |
| `GOOGLE_POLLEN_KEY` | Yes (for pollen proxy) | Google Pollen API key for the `/api/pollen` endpoint. |

To add or update variables in Vercel:

1. Go to the Vercel dashboard → `clickyweather-proxy` project
2. Settings → Environment Variables
3. Add/update the variable with the raw (unencoded) value
4. Set target environment to **Production**
5. **Redeploy** — env vars only take effect after a new deployment

---

## Testing Auth on the Live Endpoint

After redeploying, verify pollen proxy auth with:

```bash
# Should return 401
curl -s -w "\nHTTP:%{http_code}" "https://clickyweather-proxy.vercel.app/api/pollen?lat=37.77&lon=-122.41"

# Should return 401
curl -s -w "\nHTTP:%{http_code}" "https://clickyweather-proxy.vercel.app/api/pollen?lat=37.77&lon=-122.41&key=wrongkey"

# Should return 200
curl -s -w "\nHTTP:%{http_code}" "https://clickyweather-proxy.vercel.app/api/pollen?lat=37.77&lon=-122.41&key=<URL-ENCODED-SECRET>"
```

---

## Build

```bash
pebble build
```

The webpack step bundles `src/pkjs/index.js` into `build/pebble-js-app.js`. The `build/` directory
is gitignored, so compiled output will not be committed.
