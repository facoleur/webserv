CCFLAGS= -g -Iinc -Wall -Wextra -Werror -std=c++98
CC = g++

RM = rm -rf
MKDIR = mkdir -p

NAME = webserv

SRCS =	main.cpp Server.cpp Config.cpp
SRC_DIR = src/
INC_DIR = inc/
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))
OBJ_DIR = bin
BIN_DIR = bin

DEF_COLOR = \033[0;39m
GRAY = \033[0;90m
RED = \033[0;91m
GREEN = \033[0;92m
YELLOW = \033[0;93m
BLUE = \033[0;94m
MAGENTA = \033[0;95m
CYAN = \033[0;96m
WHITE = \033[0;97m


all: $(NAME)

$(NAME): $(OBJS)
	@echo "Creating $@"
	@$(CC) $(CCFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)The Makefile of $(NAME) has been compied!$(DEF_COLOR)"
	@echo "$(YELLOW)Use this command in the folder root: ./$(NAME) to start\n$(DEF_COLOR)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Creating ./$@"
	@$(MKDIR) $(BIN_DIR)
	@$(CC) $(CCFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	@echo "Cleaning up $(NAME)"
	@$(RM) $(OBJ_DIR)
	@echo "$(BLUE)$(NAME) Object files cleaned!$(DEF_COLOR)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(BLUE)$(NAME) Executable files cleaned!$(DEF_COLOR)\n"

re: fclean all
	@echo "$(BLUE)$(NAME) Cleaned and re-compiled everything!$(DEF_COLOR)"

leaks:
	@leaks -atExit -- ./$(NAME)
