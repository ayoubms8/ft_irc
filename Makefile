NAME :=	ircserv
NAME_BONUS := ircserv_bonus

OBJS_DIR = objs/
BONUS_OBJS_DIR = bonus/objs/
SRCS_DIR := src/
BONUS_SRCS_DIR := bonus/srcs/

CC := c++
CFLAGS := -Wall -Wextra -Werror -std=c++98

SRC := Authenticate.cpp Channel.cpp Client.cpp Commands.cpp Server.cpp main.cpp
SRC_BONUS := Authenticate.cpp Channel.cpp Client.cpp Commands.cpp Server.cpp main.cpp Bot.cpp

OBJ	:= $(SRC:.cpp=.o)
OBJ_BONUS := $(SRC_BONUS:.cpp=.o)

OBJS := $(addprefix objs/, $(OBJ))
OBJS_BONUS := $(addprefix bonus/objs/, $(OBJ_BONUS))

.PHONY: all clean fclean re

all: $(NAME)

bonus : $(NAME_BONUS)

$(NAME): $(OBJS)
	$(CC) -g $(OBJS) -o $(NAME) -I inc
	@echo "build complete!"

$(NAME_BONUS): $(OBJS_BONUS)
	$(CC) -g $(OBJS_BONUS) -o $(NAME) -I bonus/inc
	@echo "build complete!"

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp inc/Server.hpp inc/Client.hpp inc/Channel.hpp
	@mkdir -p $(OBJS_DIR)
	$(CC) -g $(CFLAGS) -c $< -o $@

$(BONUS_OBJS_DIR)%.o: $(BONUS_SRCS_DIR)%.cpp bonus/inc/Server.hpp bonus/inc/Client.hpp bonus/inc/Channel.hpp bonus/inc/Bot.hpp
	@mkdir -p $(BONUS_OBJS_DIR)
	$(CC) -g $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJS_DIR) $(BONUS_OBJS_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all