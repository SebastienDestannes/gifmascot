#include "gifdec.h"
#include "stdlib.h"
#include "gifmascot.h"

uint32_t CountGifFrames(gd_GIF *gif) {
  uint32_t n = 0;
  while (gd_get_frame(gif))
    n++;
  gd_rewind(gif);
  return n;
}

unsigned char **CreateGifImages(gd_GIF *gif, int n_frames) {
  uint8_t *frame = malloc(gif->width * gif->height * 3);
  uint8_t **gifImages = malloc(n_frames * sizeof(uint8_t *));
  for (int i = 0; i < n_frames; i++) {
    uint8_t *image = malloc(gif->width * gif->height * 4);
    if (!gd_get_frame(gif))
      break;
    gd_render_frame(gif, frame);
    uint8_t *p = image;
    uint8_t *color = frame;
    for (int j = 0; j < gif->height * gif->width; j++) {
      if (!gd_is_bgcolor(gif, color)) {
        *p++ = color[2]; // blue
        *p++ = color[1]; // green
        *p++ = color[0]; // red
        *p++ = 255;
      } else {
        *p++ = 0; // blue
        *p++ = 0; // green
        *p++ = 0; // red
        *p++ = 0;
      }
      color += 3;
    }
    gifImages[i] = image;
  }
  return gifImages;
}