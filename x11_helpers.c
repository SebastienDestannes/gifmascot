#include "gifmascot.h"

void DrawRightClickMenu(Display *dpy, Window menu, GC gc,
                        const char **menu_items, int menu_count,
                        int highlighted) {
  XWindowAttributes wa;
  XGetWindowAttributes(dpy, menu, &wa);

  for (int i = 0; i < menu_count; i++) {
    if (i == highlighted) {
      XSetForeground(dpy, gc, 0x3377FF);
      XFillRectangle(dpy, menu, gc, 0, i * MENU_ITEM_HEIGHT, MENU_WIDTH,
                     MENU_ITEM_HEIGHT);
      XSetForeground(dpy, gc, 0xFFFFFF);
    } else {
      XSetForeground(dpy, gc, 0xF0F0F0);
      XFillRectangle(dpy, menu, gc, 0, i * MENU_ITEM_HEIGHT, MENU_WIDTH,
                     MENU_ITEM_HEIGHT);
      XSetForeground(dpy, gc, 0x000000);
    }
    XDrawString(dpy, menu, gc, 8, i * MENU_ITEM_HEIGHT + 17, menu_items[i],
                strlen(menu_items[i]));
  }
}

XImage *CreatePixmapImage(Display *display, Visual *visual, void **data,
                          int width, int height) {
  static XShmSegmentInfo shminfo;
  XImage *image =
      XShmCreateImage(display, visual, 32, ZPixmap, 0, &shminfo, width, height);
  shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
                         IPC_CREAT | 0777);
  shminfo.shmaddr = image->data = *data = shmat(shminfo.shmid, 0, 0);
  shminfo.readOnly = False;
  XShmAttach(display, &shminfo);
  return image;
}

t_coord GetWindowPos(Display *display, Window window) {
  Window child;
  XWindowAttributes attrs;
  t_coord coord;
  XGetWindowAttributes(display, window, &attrs);
  XTranslateCoordinates(display, window, RootWindow(display, 0), attrs.x,
                        attrs.y, &coord.x, &coord.y, &child);
  return coord;
}

t_coord GetWindowGeometry(Display *dpy, Window win) {
  Window child;
  XWindowAttributes attrs;
  t_coord coord;
  XGetWindowAttributes(dpy, win, &attrs);
  XTranslateCoordinates(dpy, win, RootWindow(dpy, 0),
                        attrs.x, attrs.y,
                        &coord.x, &coord.y, &child);
  return coord;
}

void ProcessEvent(Display *display, Window window, t_config config) {
  XEvent ev;
  if (XPending(display)) {
    XNextEvent(display, &ev);
    switch (ev.type) {
    case Expose:
      break;
    case ButtonPress:
      UpdateConfigFile(config, GetWindowPos(display, window));
      printf("Saving config!\n");
      break;
    }
  }
}

// Equivalent C de : xprop -root _NET_ACTIVE_WINDOW
// Retourne la fenêtre active au moment du lancement.
// Fallback sur $WINDOWID si la propriété est absente (WM sans EWMH).
Window GetParentTerminalWindow(Display *dpy) {
  // Essai 1 : $WINDOWID (alacritty, xterm, urxvt…)
  char *wid_str = getenv("WINDOWID");
  if (wid_str && wid_str[0])
    return (Window)strtoul(wid_str, NULL, 10);

  // Essai 2 : _NET_ACTIVE_WINDOW sur la root (EWMH, tous les WM modernes)
  Atom net_active = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
  if (net_active == None)
    return 0;

  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), net_active,
                         0, 1, False, XA_WINDOW,
                         &actual_type, &actual_format,
                         &nitems, &bytes_after, &prop) != Success || !prop)
    return 0;

  Window active = *(Window *)prop;
  XFree(prop);
  return active;
}

// Equivalent C du -frame de xwininfo :
// remonte au frame du WM (parent direct de la root) pour avoir
// les coordonnées et dimensions réelles avec décorations.
// Remplit x, y dans un t_coord et stocke width/height dans out_w/out_h.
t_coord GetFrameGeometry(Display *dpy, Window win,
                         unsigned int *out_w, unsigned int *out_h) {
  Window root = DefaultRootWindow(dpy);
  Window frame = win;
  Window parent;
  Window *children;
  unsigned int nchildren;
  t_coord coord = {0, 0};

  // Remonter jusqu'au frame du WM (son parent direct est la root)
  while (1) {
    Window dummy_root;
    if (!XQueryTree(dpy, frame, &dummy_root, &parent, &children, &nchildren))
      break;
    XFree(children);
    if (parent == root)
      break;
    frame = parent;
  }

  XWindowAttributes attrs;
  XGetWindowAttributes(dpy, frame, &attrs);

  Window child;
  XTranslateCoordinates(dpy, frame, root,
                        attrs.x, attrs.y,
                        &coord.x, &coord.y, &child);
  if (out_w) *out_w = attrs.width;
  if (out_h) *out_h = attrs.height;
  return coord;
}

// Black magic : fenêtre transparente sans décoration de WM.
Window CreateTransparentWindow(t_config config, Display *display,
                               XVisualInfo *vinfo, GC *gc,
                               int width, int height) {
  XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, vinfo);
  Colormap colormap = XCreateColormap(display, RootWindow(display, 0),
                                      vinfo->visual, AllocNone);
  XSetWindowAttributes attrs = {
      .colormap = colormap, .border_pixel = 0, .background_pixel = 0};
  Window window =
      XCreateWindow(display, RootWindow(display, 0), config.x, config.y,
                    width, height, 0, vinfo->depth, InputOutput, vinfo->visual,
                    CWColormap | CWBorderPixel | CWBackPixel, &attrs);
  struct {
    unsigned long flags, functions, decorations;
    long input_mode;
    unsigned long status;
  } hints = {2, 0, 0, 0, 0};
  Atom motif_hints  = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  Atom wm_state     = XInternAtom(display, "_NET_WM_STATE", False);
  Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  Atom skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
  XChangeProperty(display, window, wm_state, XA_ATOM, 32, PropModeReplace,
                  (uint8_t *)&wm_state_above, 1);
  XChangeProperty(display, window, wm_state, XA_ATOM, 32, PropModeAppend,
                  (uint8_t *)&skip_taskbar, 1);
  XChangeProperty(display, window, motif_hints, motif_hints, 32,
                  PropModeReplace, (uint8_t *)&hints, 5);
  XSelectInput(display, window,
               ButtonPressMask | ExposureMask | KeyPressMask);
  XMapWindow(display, window);
  XSizeHints posHints = {.flags = PPosition, .x = config.x, .y = config.y};
  XSetWMNormalHints(display, window, &posHints);
  *gc = XCreateGC(display, window, 0, NULL);
  return window;
}