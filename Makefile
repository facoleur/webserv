.PHONY: all clean fclean re leaks debug conftest
.PHONY: all clean fclean re leaks debug conftest

NAME = webserv
CC = c++
CCFLAGS = -Wall -Werror -Wextra -std=c++98 -MMD -MP

RM = rm -rf
MKDIR = mkdir -p

NAME = webserv


SOURCE_FILES = main Server Request RequestParser RequestUtils Utils ConfigParser Response RequestRouter Config

HEADERS = Config.hpp ConfigParser.hpp Request.hpp RequestParser.hpp Server.hpp Webserv.hpp utils.hpp Response.hpp RequestRouter.hpp

OBJ_DIR = build/
SRC_DIR = src/
INC_DIR = inc/

SRCS = $(addprefix $(SRC_DIR), $(addsuffix .cpp, $(SOURCE_FILES)))
OBJS = $(addprefix $(OBJ_DIR), $(addsuffix .o, $(SOURCE_FILES)))
DEPS = $(OBJS:.o=.d)

DEF_COLOR = \033[0;39m
GRAY = \033[0;90m
RED = \033[0;91m
GREEN = \033[0;92m
YELLOW = \033[0;93m
BLUE = \033[0;94m
MAGENTA = \033[0;95m
CYAN = \033[0;96m
WHITE = \033[0;97m

all: $(OBJ_DIR) $(NAME)

$(NAME): $(OBJS)
	@echo "Creating $@"
	@$(CC) $(CCFLAGS) -I$(INC_DIR) $(OBJS) -o $(NAME)
	@echo "$(GREEN)The Makefile of $(NAME) has been compied!$(DEF_COLOR)"
	@echo "$(YELLOW)Use this command in the folder root: ./$(NAME) to start\n$(DEF_COLOR)"

$(OBJ_DIR):
	@echo "Creating $(NAME) build directory"
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@echo "Creating ./$@"
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
CONF_NEGFILTER ?= *.conf
PARSER_BIN := ./webserv
VERBOSE = 1
TEST_DIR := ./tests
TEST_CONF_FILE := run_config_parser_tests.sh

conftest:
	@CONF_DIR="$(CONF_DIR)" CONF_FILES="$(CONF_FILES)" PARSER_BIN="$(PARSER_BIN)" VERBOSE="$(VERBOSE)" \
		CONF_FILTER="$(CONF_FILTER)" CONF_NEGFILTER="$(CONF_NEGFILTER)" \
		$(TEST_DIR)/$(TEST_CONF_FILE)


-include $(DEPS)

# Usage:
#   make test CONF_FILTER=default.conf
#   make test CONF_FILES="./conf/default.conf ./conf/basic.conf"
# Run all positives + built-in negative checks:
# make conftest
# Only test specific negative files (skip positives):
# make conftest CONF_NEGFILTER='invalid_*.conf'
# Combine with a positive filter:
# make conftest CONF_FILTER='*.conf' CONF_NEGFILTER='missing_*.conf'
