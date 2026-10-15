# Ry Apps

The site for my Windows apps, [RyHome](https://github.com/ryanpolasky/RyHome) and [RyMessage](https://github.com/ryanpolasky/RyMessage). Live at [apps.ryanpolasky.com](https://apps.ryanpolasky.com).

It's a single static page built with Vite and served as a Cloudflare Worker with static assets.

## Development

```
npm install
npm run dev
```

## Deploy

```
npm run deploy
```

That builds to `dist/` and runs `wrangler deploy`, which also attaches the `apps.ryanpolasky.com` custom domain from `wrangler.jsonc`. Run `npx wrangler login` once first.

Screenshots in `public/shots/` come from each app's demo mode, so they never show real data.

## Disclaimer

These are independent personal projects, not affiliated with, endorsed by, sponsored by, or associated with Apple Inc., Google, Amazon, or the Connectivity Standards Alliance. Apple, iMessage, Messages, Apple Home, Mac, and macOS are trademarks of Apple Inc., and Matter is a trademark of the Connectivity Standards Alliance. Names are only used to describe what the software works with.
