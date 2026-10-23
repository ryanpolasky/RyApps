# RyApps

The site for my Windows apps, [RyHome](https://github.com/ryanpolasky/RyHome), [RyMessage](https://github.com/ryanpolasky/RyMessage), and [RyCenter](native/rycenter) (preview). Live at [apps.ryanpolasky.com](https://apps.ryanpolasky.com).

It's a single static page built with Vite and served as a Cloudflare Worker with static assets.

RyCenter's standalone C++/Win32 source, MIT license, build instructions, and tests live in [`native/rycenter`](native/rycenter). It has no dependency on the site or Node.js. Its GitHub Actions workflow builds a portable Windows x64 preview; the site links to that workflow rather than advertising an unpublished release. The RyCenter graphic is explicitly an illustration, not a screenshot.

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

RyHome's screenshots exclude unsupported camera, doorbell, Apple TV, HomePod, and robot vacuum placeholders. Its demo sensors represent standalone Matter sensors, not HomePod readings. The site lists support limits and distinguishes hardware-tested lights from other implemented device types.

## Disclaimer

These are independent personal projects, not affiliated with, endorsed by, sponsored by, or associated with Apple Inc., Google, Amazon, or the Connectivity Standards Alliance. Apple, iMessage, Messages, Apple Home, Mac, and macOS are trademarks of Apple Inc., and Matter is a trademark of the Connectivity Standards Alliance. Names are only used to describe what the software works with.
