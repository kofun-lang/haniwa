# Design

This document fixes the decisions that the issues then implement. It is written
to be argued with: if a claim here is wrong, changing it is a pull request
against this file, not a comment in a thread.

## 1. Process model

One host process. It owns the window, the web view, the WebRTC stack, and the
capability table. The web view is a child of the host and has no direct access
to the operating system beyond what the host grants.

```
┌──────────────────────────── host process ─────────────────────────────┐
│  window + menus        capability table       WebRTC engine           │
│        │                      │                    │                  │
│        └──── IPC bridge ──────┴────────────────────┘                  │
│                    │                                                  │
│            ┌───────┴────────┐                                         │
│            │  system webview │  ← renders only                        │
│            └────────────────┘                                         │
└───────────────────────────────────────────────────────────────────────┘
```

There is no second process per window in v1. Process-per-window is a real
isolation win and a real memory cost; it is deferred until someone shows an app
that needs it.

## 2. The web view is not the runtime

The system web view is used for layout, painting, and input. Everything else —
files, network, media devices, peer connections, the update channel — lives in
the host and is reachable only through the bridge.

The consequence that matters: **a capability the web view was never granted
cannot be reached by any JavaScript running in it**, including JavaScript that
arrived via an XSS in the app's own page. This is the property that makes a
web-view app defensible, and Haniwa gives it up nowhere.

## 3. WebRTC in the host

### The problem

Platform web views disagree about WebRTC in ways that break applications:

- **WebKitGTK** (Linux): `getDisplayMedia` support varies by version and portal
  configuration; H.264 availability depends on the distribution's build.
- **WebView2** (Windows): tracks Edge, so behaviour changes under the app.
- **WKWebView** (macOS): capture requires entitlements that interact with the
  app's sandbox and notarisation.

Every serious call or screen-share app on a web-view shell eventually ships its
own Chromium to escape this, which forfeits the size and memory argument.

### The decision

The peer connection lives in the host process. The host implements or embeds:

- ICE (STUN, TURN, trickle), with a pluggable candidate policy
- DTLS-SRTP for media, SCTP for data channels
- codec negotiation with an explicit, inspectable list per platform
- capture: camera, microphone, display, and application-window sources

The web view sees a bridge object whose surface mirrors `RTCPeerConnection`,
`RTCDataChannel`, and `MediaStream` closely enough that existing signalling code
ports without rewriting, and it renders incoming tracks through a host-provided
sink rather than a browser-managed one.

### What this buys

- identical negotiation on all three platforms, testable in CI without a browser
- Kofun code can hold, fork, record, or forward a stream without a JavaScript
  round trip
- data channels are usable with no web view open at all — a headless Haniwa
  process is a valid peer

### What this costs

- a media stack to maintain, which is the largest single item in this repository
- larger binary than a shell with no WebRTC, which is why it is a separate
  feature and the size gate has two numbers

## 4. Size gate

Two numbers, enforced in CI on every platform, failing the build when exceeded:

| Build | Ceiling |
|---|---|
| hello world, no WebRTC | 2 MB |
| WebRTC enabled | 6 MB |

Rules that keep them honest: no bundled browser engine, no bundled script
runtime, no runtime toolchain dependency, release builds with symbols stripped
and LTO on. If a feature cannot fit, it becomes opt-in at build time rather than
quietly raising the ceiling.

## 5. IPC

Commands are declared once, in Kofun, and both sides are generated:

- host side: a dispatch table with typed arguments and a `Result` return
- web side: a typed client, plus a `.d.ts` so TypeScript users get errors at
  build time rather than at call time

Rules:

- every command names the capabilities it requires; calling without them is a
  typed error, not a silent no-op
- payloads are size-limited by default, because an unbounded bridge is a
  denial-of-service surface reachable from a compromised page
- streaming responses are first class — a command may return a channel, so file
  reads and media do not have to be chunked by hand

## 6. Capabilities

A window is created with an explicit allowlist. There is no ambient authority
and no "dev mode grants everything" switch — development uses a checked-in
development profile so what you tested is what you shipped.

Capability names are coarse enough to audit and fine enough to be meaningful:
`fs:read:<scope>`, `fs:write:<scope>`, `net:connect:<host-pattern>`,
`media:camera`, `media:microphone`, `media:display`, `shell:open`,
`notification`, `clipboard:read`, `clipboard:write`.

The manifest is the source of truth, it ships inside the signed bundle, and the
host refuses to start if the manifest and the signature disagree.

## 7. Authentication

Desktop OAuth has two workable redirect strategies, and Haniwa supports both
because platforms disagree about which is available:

- **loopback redirect** — the host binds `127.0.0.1` on an ephemeral port and
  tears it down after one request
- **custom scheme deep link** — registered at install time, delivered to the
  running instance

Both require PKCE. Tokens go to the platform keychain — Keychain, Credential
Manager, Secret Service — and never to the web view. The web view receives
whatever session identifier the app chooses, not the refresh token. The protocol
work lives in [kofun-auth](https://github.com/kofun-lang/kofun-auth); Haniwa
owns only the desktop-specific delivery.

## 8. Staging against the language

Kofun's executable slice cannot yet express a window loop or a media stack. So:

- **Stage 0** — host in C with a stable C ABI, Kofun defines the command
  surface and the manifest schema. Real apps are possible; most of the host is
  not Kofun.
- **Stage 1** — command dispatch, capability checks, and the update client move
  into Kofun as records, results, and collections land.
- **Stage 2** — the WebRTC state machine moves into Kofun. The codec and
  transport layers may stay native forever; that is a fine outcome.

Every issue states which stage it belongs to. An issue that cannot be done in
Kofun today says so and names the blocking diagnostic, so the work doubles as
pressure on the compiler.
