NAME = civilization
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Isrc

# Locate ncurses in the Nix store if not in standard paths
NCURSES_INC := $(shell find /nix/store -maxdepth 4 -name "ncurses.h" 2>/dev/null | head -1 | xargs dirname 2>/dev/null)
NCURSES_LIB := $(shell find /nix/store -maxdepth 4 -name "libncurses.so" 2>/dev/null | head -1 | xargs dirname 2>/dev/null)

ifneq ($(NCURSES_INC),)
CFLAGS += -I$(NCURSES_INC)
endif

LDFLAGS :=
ifneq ($(NCURSES_LIB),)
LDFLAGS += -L$(NCURSES_LIB)
endif
LDFLAGS += -lncurses

SRC = $(shell find src -name '*.c')
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LDFLAGS)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
