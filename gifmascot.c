#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "gifdec.h"

typedef struct s_config {
    int y;
    int x;
    float delta;
    char path[100];
} t_config;

uint32_t count_gif_frames(gd_GIF *gif) {
    uint32_t n = 0;
    while (gd_get_frame(gif))
        n++;
    gd_rewind(gif);
    return n;
}

t_config load_config_file() {
    char line[256];
    t_config conf = {0, 0, 0.01, ""};
    char *home = getenv("HOME");
    char *filepath = "/.config/gifpet.conf";
    strlcpy(conf.path, home, 50);
    strlcat(conf.path, filepath, 50);
    FILE* fp = fopen(conf.path, "r");
    if (!fp)
        return conf;
    fgets(line, 10, fp);
    conf.x = atoi(line);
    fgets(line, 10, fp);
    conf.y = atoi(line);
    fgets(line, 10, fp);
    conf.delta = atof(line);
    fclose(fp);
    return conf;
}

void update_config_file(t_config conf, int x, int y, float speed) {
    FILE* fp = fopen(conf.path, "w");
    char buf[50];
    if (!fp)
        return;
    sprintf(buf, "%d\n", x);
    fputs(buf, fp);
    sprintf(buf, "%d\n", y);
    fputs(buf, fp);
    sprintf(buf, "%f\n", speed);
    fputs(buf, fp);
    fclose(fp);
}

XImage *CreatePixmapImage(Display *display, Visual *visual, void ** data, int width, int height) {
	static XShmSegmentInfo shminfo;
	XImage *image = XShmCreateImage(display, visual, 32, ZPixmap, 0, &shminfo, width, height);
	shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,IPC_CREAT|0777);
	shminfo.shmaddr = image->data = *data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
	XShmAttach(display, &shminfo);
	return image;
}

unsigned char** createGifImages(gd_GIF *gif, int n_frames) {
    uint8_t *frame = malloc(gif->width * gif->height * 3);
    uint8_t **gifImages = malloc(n_frames * sizeof(uint8_t *));
    for (int i=0; i < n_frames; i++) {
        uint8_t *image = malloc(gif->width * gif->height * 4);
        if(!gd_get_frame(gif))
            break;
        gd_render_frame(gif, frame);
        uint8_t *p = image;
        uint8_t *color = frame;
        for (int i = 0; i < gif->height * gif->width; i++) {
            if (!gd_is_bgcolor(gif, color)) {
                *p++=color[2]; // blue
                *p++=color[1]; // green
                *p++=color[0]; // red
                *p++=255;
            }
            else {
                *p++=0; // blue
                *p++=0; // green
                *p++=0; // red
                *p++=0;
            }
            color += 3;
        }
        gifImages[i] = image;
    }
    return gifImages;
}

void getWindowPos(Display *display, Window window, int *x, int *y) {
    Window child;
    XWindowAttributes attrs;
    XGetWindowAttributes(display, window, &attrs);
    XTranslateCoordinates(display, window, RootWindow(display, 0),
        attrs.x, attrs.y, x, y, &child);
}

void processEvent(Display *display, Window window, t_config config) {
    XEvent ev;
    int x, y;

    if (XPending(display)) {
		XNextEvent(display, &ev);
		switch(ev.type) {
			case Expose:		break;
			case ButtonPress:
			    getWindowPos(display, window, &x, &y);
				update_config_file(config, x, y, config.delta);
			    printf("Saving config!\n");
				break;
		}
	}
}

// This is claude black magic to create a transparent window without borders.
void CreateTransparentWindow(t_config config, Display** display, XVisualInfo* vinfo, Window* window, GC *gc, int width, int height) {
    *display = XOpenDisplay(NULL);
    XMatchVisualInfo(*display, DefaultScreen(*display), 32, TrueColor, vinfo);
    Colormap colormap = XCreateColormap(*display, RootWindow(*display, 0), vinfo->visual, AllocNone);
    XSetWindowAttributes attrs = { .colormap = colormap, .border_pixel = 0, .background_pixel = 0 };
    *window = XCreateWindow(*display, RootWindow(*display, 0),
        config.x, config.y, width, height, 0,
        vinfo->depth, InputOutput, vinfo->visual,
        CWColormap | CWBorderPixel | CWBackPixel, &attrs);
    struct { unsigned long flags, functions, decorations; long input_mode; unsigned long status; } hints = {2, 0, 0, 0, 0};
    Atom motif_hints = XInternAtom(*display, "_MOTIF_WM_HINTS", False);
    Atom wm_state = XInternAtom(*display, "_NET_WM_STATE", False);
    Atom wm_state_above = XInternAtom(*display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(*display, *window, wm_state, XA_ATOM, 32,
        PropModeReplace, (uint8_t *)&wm_state_above, 1);
    XChangeProperty(*display, *window, motif_hints, motif_hints, 32,
        PropModeReplace, (uint8_t *)&hints, 5);
    XSelectInput(*display, *window, ButtonPressMask|ExposureMask|KeyPressMask);
    XMapWindow(*display, *window);
    XSizeHints posHints = { .flags = PPosition, .x = config.x, .y = config.y };
    XSetWMNormalHints(*display, *window, &posHints);
    *gc = XCreateGC(*display, *window, 0, NULL);
}

int main(int argc, char **argv)
{
    Window window;
    GC gc ;
    XVisualInfo vinfo;
    gd_GIF *gif;
    uint8_t *image;

    if (argc < 2 || !(gif = gd_open_gif(argv[1]))) {
        fprintf(stderr, "Usage:\n%s image.gif\n", argv[0]);
        return 1;
    }
    uint32_t width=gif->width;
    uint32_t height=gif->height;
    t_config config = load_config_file();
    uint32_t n_frames = count_gif_frames(gif);
    uint8_t** gifImages = createGifImages(gif, n_frames);
    Display* display = XOpenDisplay(NULL);
    CreateTransparentWindow(config, &display, &vinfo, &window, &gc, width, height);
    XImage* ximage = CreatePixmapImage(display, vinfo.visual, (void**) &image, width, height);

    uint32_t frame_count = 0;
    while(1)
    {
        if (frame_count >= n_frames)
            frame_count = 0;
        processEvent(display, window, config);
        memcpy(image, gifImages[frame_count], width * height * 4);
        XShmPutImage(display, window, gc, ximage, 0, 0, 0, 0, width, height, False);
		usleep(config.delta * 1000000);
		frame_count++;
    }
}
