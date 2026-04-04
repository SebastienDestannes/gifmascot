#include "gifmascot.h"

#define MENU_SAVE_POSITION 0
#define MENU_EXIT 1

const char *menu_items[] = {"Save position", "Exit"};
const int menu_count = sizeof(menu_items) / sizeof(char *);


Window CreateMenuWindow(Display *dpy, Window root) {
  // Menu window (initially hidden via XUnmapWindow)
  Window menu =
      XCreateSimpleWindow(dpy, root, 0, 0, MENU_WIDTH,
                          menu_count * MENU_ITEM_HEIGHT, 1, 0x999999, 0xF0F0F0);
  XSelectInput(dpy, menu,
               ExposureMask | ButtonPressMask | ButtonReleaseMask |
                   PointerMotionMask | LeaveWindowMask);

  // Override redirect prevents the WM from decorating the menu popup
  XSetWindowAttributes swa = {.override_redirect = True};
  XChangeWindowAttributes(dpy, menu, CWOverrideRedirect, &swa);
  return menu;
}

GC CreateMenuGc(Display *dpy, Window menu) {
  XGCValues vals;
  vals.foreground = 0x000000;
  vals.background = 0xF0F0F0;
  return XCreateGC(dpy, menu, GCForeground | GCBackground, &vals);
}

void HandleRightClickMenu(Display *dpy, Window window, Window menu, GC gc,
                          GC menu_gc, t_config config, int *menu_visible) {
  XEvent ev;
  int highlighted = -1;
  while (XPending(dpy)) {
    XNextEvent(dpy, &ev);

    if (ev.type == Expose) {
      if (ev.xexpose.window == window) {
        XDrawString(dpy, window, gc, 10, 20, "Right-click for menu", 20);
      } else if (ev.xexpose.window == menu) {
        DrawRightClickMenu(dpy, menu, menu_gc, menu_items, menu_count,
                           highlighted);
      }
    }

    // Right-click on main window → show menu
    else if (ev.type == ButtonPress && ev.xbutton.button == Button3 &&
             ev.xbutton.window == window) {
      XMoveWindow(dpy, menu, ev.xbutton.x_root, ev.xbutton.y_root);
      XMapRaised(dpy, menu);
      XGrabPointer(dpy, menu, True,
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      *menu_visible = 1;
    }

    // Click on menu → select item or dismiss
    else if (ev.type == ButtonPress && ev.xbutton.window == menu) {
      int item = ev.xbutton.y / MENU_ITEM_HEIGHT;
      if (item >= 0 && item < menu_count) {
        printf("Selected: %s\n", menu_items[item]);
        switch (item) {
            case 0:
                UpdateConfigFile(config, GetWindowPos(dpy, window));
                break;
            case 1:
                exit(0);
            default:
                break;
        }
      }
      XUngrabPointer(dpy, CurrentTime);
      XUnmapWindow(dpy, menu);
      *menu_visible = 0;
      highlighted = -1;
    }

    // Click outside menu → dismiss
    else if (ev.type == ButtonPress) {
      XUngrabPointer(dpy, CurrentTime);
      XUnmapWindow(dpy, menu);
      *menu_visible = 0;
      highlighted = -1;
    }

    // Hover highlight
    else if (ev.type == MotionNotify && ev.xmotion.window == menu) {
      int item = ev.xmotion.y / MENU_ITEM_HEIGHT;
      if (item != highlighted) {
        highlighted = item;
        DrawRightClickMenu(dpy, menu, menu_gc, menu_items, menu_count,
                           highlighted);
      }
    }

    // Mouse leaves menu → clear highlight
    else if (ev.type == LeaveNotify && ev.xcrossing.window == menu) {
      highlighted = -1;
      DrawRightClickMenu(dpy, menu, menu_gc, menu_items, menu_count,
                         highlighted);
    }
  }
}