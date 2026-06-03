NAME = civilization
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Isrc

# Locate CSFML 2.x in the Nix store
CSFML_INC := $(shell find /nix/store -maxdepth 6 -name "Graphics.h" -path "*/SFML/*" 2>/dev/null | grep "csfml-2" | head -1 | xargs dirname 2>/dev/null | xargs dirname 2>/dev/null)
CSFML_LIB := $(shell find /nix/store -maxdepth 6 -name "libcsfml-graphics.so" 2>/dev/null | grep "csfml-2" | head -1 | xargs dirname 2>/dev/null)

ifneq ($(CSFML_INC),)
CFLAGS += -I$(CSFML_INC)
endif

LDFLAGS :=
ifneq ($(CSFML_LIB),)
LDFLAGS += -L$(CSFML_LIB) -Wl,-rpath,$(CSFML_LIB)
endif
LDFLAGS += -lcsfml-graphics -lcsfml-window -lcsfml-system

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
