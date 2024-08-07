NAME :=	ircserv

OBJS_DIR = objs/
SRCS_DIR := srcs/

CC := c++
CFLAGS := -g -Wall -Wextra -Werror -std=c++98

SRC := Authenticate.cpp Channel.cpp Client.cpp Commands.cpp Server.cpp main.cpp Bot.cpp

OBJ	:= $(SRC:.cpp=.o)

OBJS := $(addprefix objs/, $(OBJ))

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME) -I inc
	@echo "build complete!"

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp inc/Server.hpp inc/Client.hpp inc/Channel.hpp inc/Bot.hpp
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJS_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re