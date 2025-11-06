.PHONY: all clean fclean re leaks debug test conftest

NAME = webserv
CC = c++
CCFLAGS = -Wall -Werror -Wextra -std=c++98

RM = rm -rf
MKDIR = mkdir -p

NAME = webserv


SRCS = main.cpp Server.cpp Request.cpp RequestParser.cpp RequestUtils.cpp utils.cpp Utils.cpp ConfigParser.cpp Response.cpp RequestRouter.cpp

HEADERS = Config.hpp ConfigParser.hpp Request.hpp RequestParser.hpp Server.hpp Webserv.hpp Utils.hpp Response.hpp RequestRouter.hpp

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

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(addprefix $(INC_DIR)/, $(HEADERS))
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

debug: CCFLAGS += -DDEBUG_MODE
debug: re

CONF_DIR := ./config
CONF_FILES ?=
CONF_FILTER ?= *.conf
#CONF_NEGFILTER ?= negative_*.conf
PARSER_BIN := ./webserv
VERBOSE = 1
TEST_DIR := ./tests
TEST_CONF_FILE := run_config_parser_tests.sh

conftest:
	@CONF_DIR="$(CONF_DIR)" CONF_FILES="$(CONF_FILES)" PARSER_BIN="$(PARSER_BIN)" VERBOSE="$(VERBOSE)" \
		CONF_FILTER="$(CONF_FILTER)" CONF_NEGFILTER="$(CONF_NEGFILTER)" \
		$(TEST_DIR)/$(TEST_CONF_FILE)

# Usage:
#   make test CONF_FILTER=default.conf
#   make test CONF_FILES="./conf/default.conf ./conf/basic.conf"
# Run all positives + built-in negative checks:
# make conftest
# Only test specific negative files (skip positives):
# make conftest CONF_NEGFILTER='invalid_*.conf'
# Combine with a positive filter:
# make conftest CONF_FILTER='*.conf' CONF_NEGFILTER='missing_*.conf'
