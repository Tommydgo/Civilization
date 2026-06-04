NAME = civilization
CC = /usr/bin/gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Isrc

# Locate Raylib via pkg-config or /usr/local fallback
RAYLIB_INC := $(shell PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config --cflags raylib 2>/dev/null | sed 's/-I//g')
RAYLIB_LIB := $(shell PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config --libs-only-L raylib 2>/dev/null | sed 's/-L//g')

ifneq ($(RAYLIB_INC),)
CFLAGS += -I$(RAYLIB_INC)
else
CFLAGS += -I/usr/local/include
endif

LDFLAGS :=
ifneq ($(RAYLIB_LIB),)
LDFLAGS += -L$(RAYLIB_LIB) -Wl,-rpath,$(RAYLIB_LIB)
else
LDFLAGS += -L/usr/local/lib -Wl,-rpath,/usr/local/lib
endif
LDFLAGS += -lraylib -lm

SRC     = $(shell find src -name '*.c')
OBJ     = $(SRC:.c=.o)

# Tests: exclude main + all UI files (render_stub.c replaces them)
TEST_GAME_SRC = $(filter-out src/main.c src/ui/render.c src/ui/ui_draw.c src/ui/ui_dialog.c, $(SRC))
TEST_GAME_OBJ = $(TEST_GAME_SRC:.c=.o)
TEST_SRC      = $(shell find tests -name '*.c')
TEST_OBJ      = $(TEST_SRC:.c=.o)
TEST_BIN      = test_runner

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LDFLAGS)

tests: $(TEST_GAME_OBJ) $(TEST_OBJ)
	$(CC) $(TEST_GAME_OBJ) $(TEST_OBJ) -o $(TEST_BIN) -Iinclude -Isrc
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TEST_BIN)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re tests
