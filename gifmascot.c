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

  // Un seul XOpenDisplay, partagé par tout le programme
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Cannot open X display\n");
    return 1;
  }

  t_config config = LoadConfigFile(dpy, width, height);
  uint32_t n_frames = CountGifFrames(gif);
  uint8_t **gifImages = CreateGifImages(gif, n_frames);

  GC gc;
  XVisualInfo vinfo;
  Window window = CreateTransparentWindow(config, dpy, &vinfo, &gc, width, height);
  Window root = DefaultRootWindow(dpy);
  Window menu = CreateMenuWindow(dpy, root);
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