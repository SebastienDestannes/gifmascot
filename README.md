# gifmascot — changes & additions

## Overview

gifmascot displays an animated GIF as a transparent, borderless overlay window on your X11 desktop. These changes add automatic positioning relative to the active terminal window, fix a double `XOpenDisplay` bug, and correct GIF transparency rendering.

---

## Changes by file

### `gifmascot.c`

- **Single `XOpenDisplay`**: the display is now opened once in `main` and passed to all functions. Previously `CreateTransparentWindow` opened a second connection internally, leaving the first one orphaned.
- **`LoadConfigFile` now receives GIF dimensions**: the GIF is opened first so that `width` and `height` are available before the config is loaded, allowing the initial position to be centered relative to the terminal.

### `gifmascot.h`

- Updated signature for `CreateTransparentWindow`: `Display **` → `Display *` (no longer opens its own connection).
- Updated signature for `LoadConfigFile`: added `uint32_t gif_w, uint32_t gif_h` parameters.
- Added declarations for `GetParentTerminalWindow(Display *dpy)`, `GetFrameGeometry(...)`.

### `x11_helpers.c`

#### `CreateTransparentWindow`
- Removed the internal `XOpenDisplay` call. Now takes `Display *display` directly instead of `Display **display`.

#### `GetParentTerminalWindow(Display *dpy)` — new
Finds the X11 window of the terminal that launched the program, using two strategies in order:
1. `$WINDOWID` environment variable — set by most terminal emulators (alacritty, xterm, urxvt…).
2. `_NET_ACTIVE_WINDOW` property on the root window — EWMH standard supported by all modern window managers. This is the equivalent of `xprop -root _NET_ACTIVE_WINDOW`.

#### `GetFrameGeometry(Display *dpy, Window win, unsigned int *out_w, unsigned int *out_h)` — new
Returns the absolute position and dimensions of a window **including WM decorations** (title bar, borders), equivalent to `xwininfo -id <wid> -frame`.

Walks up the X11 window tree from the client window until reaching the direct child of the root — that is the WM frame. Then calls `XTranslateCoordinates` to get absolute screen coordinates.

### `config.c`

#### `LoadConfigFile(Display *dpy, uint32_t gif_w, uint32_t gif_h)`
- On first launch (no config file), automatically detects the active terminal window and computes an initial position:
  - Horizontally: centered on the terminal (`pos.x/scale + tw/scale/2 - gif_w/2`).
  - Vertically: just above the title bar (`pos.y/scale + 23 - gif_h`), where 23 px is the title bar height.
  - A HiDPI scale factor of 2 is applied to convert Xlib physical pixel coordinates to logical coordinates.
- Immediately writes the computed position to `~/.config/gifpet.conf` via `UpdateConfigFile` so subsequent launches reuse it.

#### `UpdateConfigFile`
- Coordinates are divided by 2 before writing to account for the HiDPI scaling factor, keeping the saved values consistent with what `LoadConfigFile` expects on the next read.

### `gif_helpers.c`

#### `CreateGifImages` — transparency fix
The original code used `gd_is_bgcolor` to detect transparent pixels, which compares pixel RGB values against the GIF background color. This caused:
- Pixels with the same color as the background to be incorrectly made transparent (black or white patches).
- Truly transparent pixels defined by the GIF Graphic Control Extension to be missed.

The fix reads the raw palette index from `gif->frame[j]` and compares it against `gif->gce.tindex` when `gif->gce.transparency` is set — this is the correct way to detect transparency as defined in the GIF89a spec.

---

## Known limitations

- The HiDPI scale factor is hardcoded to `2`. A display running at 96 DPI or with a different scale factor will need this value adjusted. A proper fix would compute it from `DisplayWidth` / `DisplayWidthMM`.
- The title bar offset is hardcoded to `23` px. This works for common WM themes but may need adjustment for others.
- `_NET_ACTIVE_WINDOW` reflects the focused window at the moment `XOpenDisplay` is called. If the focus has already moved (e.g. launching via a script), the detected window may not be the terminal.