# Haniwa

A desktop application shell for [Kofun](https://github.com/kofun-lang/kofun):
your UI is a web view, your logic is Kofun, and the binary stays small.

Haniwa (埴輪) are the clay figures set on top of a kofun. The name is the
relationship: the mound is the language, this is the thing standing on it that
people actually see.

> **Status: design and issues only.** No code yet. This repository exists to
> pin down what "lighter than Tauri, with real WebRTC" has to mean before
> anyone writes the first line. Read [docs/DESIGN.md](docs/DESIGN.md) for the
> argument, and the issues for the plan.

## Why not just use Tauri

Tauri is the right shape — system web view instead of a bundled Chromium, native
backend, small binaries — and Haniwa copies that shape deliberately. Two things
are different.

**1. WebRTC is a first-class subsystem, not an afterthought.**

Tauri inherits whatever the platform web view gives it, and on Linux
(WebKitGTK) and Windows (WebView2) that is uneven: no reliable
`getDisplayMedia` on some targets, codec availability that changes with the
distribution, no way to reach the peer connection from the native side. Apps
that need calls or screen sharing end up shipping their own browser after all,
which throws away the reason to use a web view.

Haniwa runs the peer connection in the host process — ICE, DTLS-SRTP, SCTP data
channels, and the codec negotiation — and exposes it to the web view through a
typed bridge that mirrors the `RTCPeerConnection` surface. The web view renders;
it does not have to be a WebRTC implementation. That makes behaviour identical
across macOS, Windows, and Linux, and it lets Kofun code hold a media stream
without a round trip through JavaScript.

**2. Smaller, by refusing to bundle a runtime.**

The target is a hello-world binary under 2 MB and a WebRTC-enabled binary under
6 MB, on every supported platform, with no shipped Node, no shipped Chromium,
and no shipped Rust toolchain at runtime. That number is a constraint on the
design, so it is a gate in CI, not a marketing claim.

## What it is meant to do

| Area | Intent |
|---|---|
| Windowing | native window, menus, tray, file dialogs |
| Web view | system-provided: WKWebView, WebView2, WebKitGTK |
| IPC | typed commands both ways, generated from Kofun signatures |
| WebRTC | host-side peer connection, data channels, media capture, screen share |
| Security | capability allowlist per window; no ambient filesystem or network |
| Auth | deep-link and loopback redirect handling for OAuth, via [kofun-auth](https://github.com/kofun-lang/kofun-auth) |
| Packaging | `.app`, `.msi`, `.deb`, `.AppImage`, plus signing and notarisation |
| Updates | signed differential updates, opt-in, verifiable offline |

## What it is not

- Not a browser engine. If the system web view cannot render your page, Haniwa
  will not fix that for you.
- Not a mobile framework. iOS and Android are out of scope until the desktop
  targets are real.
- Not a Tauri fork. No shared code, no compatibility promise with Tauri's
  plugin API.

## Status of the language underneath

Kofun is a research compiler and its executable slice is still small. Haniwa is
therefore explicitly staged: the host process starts as a thin native shell with
a Kofun-facing ABI, and moves into Kofun as the language reaches each capability.
Issues here say which side of that line each piece is on.

## License

Dual-licensed under [MIT](LICENSE-MIT) or [Apache-2.0](LICENSE-APACHE).
