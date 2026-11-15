# Reference review

Measured 2026-08-10. This is an architectural comparison, not a dependency
list. No source was copied from the projects below.

## Desktop approaches

| Project | Useful evidence | Haniwa decision |
|---|---|---|
| [Tauri](https://github.com/tauri-apps/tauri) | A system-web-view shell can serve real desktop applications without bundling Chromium. | Keep the shape, but keep Haniwa's C ABI, host-owned WebRTC, and explicit 2 MiB/6 MiB gates. Do not promise Tauri plugin compatibility. |
| [Electron](https://github.com/electron/electron) | Bundled Chromium and Node give one browser/runtime across platforms. | Reject for the shell: that consistency contradicts the binary-size and no-bundled-runtime constraints. |
| [Flutter](https://github.com/flutter/flutter) | One retained UI and rendering stack can target desktop, mobile, and web. | Do not turn Stage 0 into a second UI toolkit. Haniwa delegates layout and painting to the installed web view. Mobile remains out of scope. |
| [Wox](https://github.com/Wox-launcher/Wox) | Its 2026 Go UI keeps portable contracts in one process and puts Win32, AppKit, and GTK details behind platform adapters. System WebView previews similarly expose one facade over WebView2, WKWebView, and WebKitGTK. | Adopt the boundary: portable lifecycle/resource contracts, thin OS adapters, host-owned event loop, and no native engine types above the adapter. Do not adopt its native widget/renderer stack. |
| [Ferrite](https://github.com/OlaProeis/Ferrite) and other egui/native apps | A compact native renderer is a viable alternative when the application is an editor or tool rather than a web UI. | Useful comparison, not a base. Haniwa exists specifically for web-front-end applications. |

Wox is GPL-3.0 and Haniwa is MIT/Apache-2.0. The comparison above uses only
public architectural facts. Haniwa's implementation is independent and does
not copy Wox code.

The first Haniwa slice follows the portable-boundary conclusion directly:
`haniwa_resource_for_uri` owns the embedded resource contract, while the three
platform files only translate `haniwa://` requests and native lifecycle events.

## BEAM and Nova

[Nova](https://github.com/novaframework/nova) is an OTP-native web framework,
not a desktop shell. Its relevant lesson is ownership: an application is a
real supervised runtime from the start, controllers return explicit values,
and pre/post request plugins are separate from controller logic. The
[Nova routing documentation](https://hexdocs.pm/nova/routing.html) also keeps
route security callbacks and plugins at the dispatch boundary.

Haniwa applies that principle narrowly:

- the host owns the event loop and window lifecycle;
- platform callbacks translate into one portable contract;
- later capability checks belong at IPC dispatch, not scattered through each
  command body;
- a failed custom-scheme load is an explicit smoke-test failure.

Haniwa does not embed the BEAM and does not depend on Nova. A future Kofun web
or service framework may profit more directly from OTP supervision, releases,
WebSocket processes, and failure isolation.

## What remains deliberately unresolved

- Wox validates that thin platform adapters scale, but its mature UI is much
  larger than Haniwa Stage 0 and does not prove Haniwa's size ceiling.
- System web views still differ. Host-side WebRTC is intended to remove one
  important difference; it does not make DOM/CSS engines identical.
- Packaging, signing, accessibility, IME, and update behavior need their own
  measured gates. A working hello-world window does not settle them.
