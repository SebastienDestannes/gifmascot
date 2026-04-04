#include "gifmascot.h"

void DrawRightClickMenu(Display *dpy, Window menu, GC gc,
                        const char **menu_items, int menu_count,
                        int highlighted) {
  XWindowAttributes wa;
  XGetWindowAttributes(dpy, menu, &wa);

  for (int i = 0; i < menu_count; i++) {
    if (i == highlighted) {
      XSetForeground(dpy, gc, 0x3377FF); // highlight color
      XFillRectangle(dpy, menu, gc, 0, i * MENU_ITEM_HEIGHT, MENU_WIDTH,
                     MENU_ITEM_HEIGHT);
      XSetForeground(dpy, gc, 0xFFFFFF); // white text on highlight
    } else {
      XSetForeground(dpy, gc, 0xF0F0F0); // background
      XFillRectangle(dpy, menu, gc, 0, i * MENU_ITEM_HEIGHT, MENU_WIDTH,
                     MENU_ITEM_HEIGHT);
      XSetForeground(dpy, gc, 0x000000); // black text
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

// This is claude black magic to create a transparent window without borders.
Window CreateTransparentWindow(t_config config, Display **display,
                             XVisualInfo *vinfo, GC *gc,
                             int width, int height) {
  *display = XOpenDisplay(NULL);
  XMatchVisualInfo(*display, DefaultScreen(*display), 32, TrueColor, vinfo);
  Colormap colormap = XCreateColormap(*display, RootWindow(*display, 0),
                                      vinfo->visual, AllocNone);
  XSetWindowAttributes attrs = {
      .colormap = colormap, .border_pixel = 0, .background_pixel = 0};
  Window window =
      XCreateWindow(*display, RootWindow(*display, 0), config.x, config.y,
                    width, height, 0, vinfo->depth, InputOutput, vinfo->visual,
                    CWColormap | CWBorderPixel | CWBackPixel, &attrs);
  struct {
    unsigned long flags, functions, decorations;
    long input_mode;
    unsigned long status;
  } hints = {2, 0, 0, 0, 0};
  Atom motif_hints = XInternAtom(*display, "_MOTIF_WM_HINTS", False);
  Atom wm_state = XInternAtom(*display, "_NET_WM_STATE", False);
  Atom wm_state_above = XInternAtom(*display, "_NET_WM_STATE_ABOVE", False);
  Atom skip_taskbar =
      XInternAtom(*display, "_NET_WM_STATE_SKIP_TASKBAR", False);
  XChangeProperty(*display, window, wm_state, XA_ATOM, 32, PropModeReplace,
                  (uint8_t *)&wm_state_above, 1);
  XChangeProperty(*display, window, wm_state, XA_ATOM, 32, PropModeAppend,
                  (uint8_t *)&skip_taskbar, 1);
  XChangeProperty(*display, window, motif_hints, motif_hints, 32,
                  PropModeReplace, (uint8_t *)&hints, 5);
  XSelectInput(*display, window,
               ButtonPressMask | ExposureMask | KeyPressMask);
  XMapWindow(*display, window);
  XSizeHints posHints = {.flags = PPosition, .x = config.x, .y = config.y};
  XSetWMNormalHints(*display, window, &posHints);
  *gc = XCreateGC(*display, window, 0, NULL);
  return window;
}
