#include "gifdec.h"
#include "gifmascot.h"

int main(int argc, char **argv) {
  gd_GIF *gif;
  uint8_t *image;

  if (argc < 2 || !(gif = gd_open_gif(argv[1]))) {
    fprintf(stderr, "Usage:\n%s image.gif\n", argv[0]);
    return 1;
  }

  uint32_t width = gif->width, height = gif->height;
  t_config config = LoadConfigFile();
  uint32_t n_frames = CountGifFrames(gif);
  uint8_t **gifImages = CreateGifImages(gif, n_frames);
  Display *dpy = XOpenDisplay(NULL);
  GC gc;
  XVisualInfo vinfo;
  Window window = CreateTransparentWindow(config, &dpy, &vinfo, &gc, width, height);
  Window root = DefaultRootWindow(dpy);
  Window menu = CreateMenuWindow(dpy, root);
  // Menu window (initially hidden via XUnmapWindow)
  XImage *ximage =
      CreatePixmapImage(dpy, vinfo.visual, (void **)&image, width, height);

  int menu_visible = 0;
  GC menu_gc = CreateMenuGc(dpy, menu);
  uint32_t frame_count = 0;
  while (1) {
    HandleRightClickMenu(dpy, window, menu, gc, menu_gc, config, &menu_visible);
    if (frame_count >= n_frames)
      frame_count = 0;
    if (!menu_visible) {
      memcpy(image, gifImages[frame_count], width * height * 4);
      XShmPutImage(dpy, window, gc, ximage, 0, 0, 0, 0, width, height, False);
    }
    usleep(gif->gce.delay * 10000);
    frame_count++;
  }
}
