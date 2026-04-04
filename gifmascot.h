#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "gifdec.h"

#define MENU_ITEM_HEIGHT 25
#define MENU_WIDTH 150

typedef struct s_config {
  int y;
  int x;
  float delta;
  char path[100];
} t_config;

typedef struct s_coord {
    int x;
    int y;
} t_coord;

// x11_helpers.c
void DrawRightClickMenu(Display *dpy, Window menu, GC gc,
                        const char **menu_items, int menu_count,
                        int highlighted);
XImage *CreatePixmapImage(Display *display, Visual *visual, void **data,
                          int width, int height);
t_coord GetWindowPos(Display *display, Window window);
void ProcessEvent(Display *display, Window window, t_config config);
Window CreateTransparentWindow(t_config config, Display **display,
                             XVisualInfo *vinfo, GC *gc,
                             int width, int height);
// gif_helpers.c
uint32_t CountGifFrames(gd_GIF *gif);
unsigned char **CreateGifImages(gd_GIF *gif, int n_frames);

// config.c
t_config LoadConfigFile();
void UpdateConfigFile(t_config conf, t_coord coord);

// menuwindow.c
Window CreateMenuWindow(Display *dpy, Window root);
GC CreateMenuGc(Display *dpy, Window menu);
void HandleRightClickMenu(Display *dpy, Window window, Window menu, GC gc,
                          GC menu_gc, t_config config, int *menu_visible);