CC = gcc
CFLAGS = -Wall -Werror -Wextra# -g -fsanitize=address
LDFLAGS = -lX11 -lXext -lm

SRC = gifmascot.c gifdec.c

all: gifmascot

gifmascot: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o gifmascot

clean:
	rm -f gifmascot

re: clean all
