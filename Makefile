CC = gcc
CFLAGS = -Wall -Werror -Wextra# -g -fsanitize=address
LDFLAGS = -lX11 -lXext -lm

SRC = gifmascot.c gifdec.c config.c x11_helpers.c gif_helpers.c menuwindow.c

all: gifmascot

gifmascot: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o gifmascot

clean:
	rm -f gifmascot

re: clean all
