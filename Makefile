NAME = civilization
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Isrc

SRC = $(shell find src -name '*.c')
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
