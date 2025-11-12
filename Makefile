.PHONY: all clean fclean re leaks debug conftest tests

MAKEFLAGS += --no-builtin-rules

NAME := webserv
CC := c++
CCFLAGS := -Wall -Werror -Wextra -std=c++98

RM := rm -rf
MKDIR := mkdir -p

SRC_DIR := src
INC_DIR := inc
OBJ_DIR := bin

SRCS := main.cpp ConfigFile.cpp Config.cpp ConfigParser.cpp Request.cpp RequestParser.cpp \
        RequestRouter.cpp RequestUtils.cpp Response.cpp Server.cpp Utils.cpp
HEADERS := ConfigFile.hpp Config.hpp ConfigParser.hpp Request.hpp RequestParser.hpp \
           RequestRouter.hpp Response.hpp Server.hpp Utils.hpp Webserv.hpp

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

# Colors
DEF_COLOR = \033[0;39m
GRAY = \033[0;90m
RED = \033[0;91m
GREEN = \033[0;92m
YELLOW = \033[0;93m
BLUE = \033[0;94m

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(YELLOW)Linking $(NAME)...$(DEF_COLOR)"
	@$(CC) $(CCFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)Build complete. Run ./$(NAME)$(DEF_COLOR)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(addprefix $(INC_DIR)/,$(HEADERS))
	@$(MKDIR) $(OBJ_DIR)
	@echo "$(GRAY)Compiling $<$(DEF_COLOR)"
	@$(CC) $(CCFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	@echo "$(BLUE)Cleaning object files...$(DEF_COLOR)"
	@$(RM) $(OBJ_DIR)

fclean: clean
	@echo "$(BLUE)Cleaning binary...$(DEF_COLOR)"
	@$(RM) $(NAME)

re: fclean all

leaks:
	@leaks -atExit -- ./$(NAME)

debug: CCFLAGS += -DDEBUG_MODE
debug: re

# Config parser tests
CONF_DIR := ./config
CONF_FILES ?=
CONF_FILTER ?= *.conf
PARSER_BIN := ./webserv
VERBOSE := 1
TEST_DIR := ./tests
TEST_CONF_FILE := run_config_parser_tests.sh

conftest:
	@CONF_DIR="$(CONF_DIR)" CONF_FILES="$(CONF_FILES)" PARSER_BIN="$(PARSER_BIN)" VERBOSE="$(VERBOSE)" \
	CONF_FILTER="$(CONF_FILTER)" CONF_NEGFILTER="$(CONF_NEGFILTER)" \
	$(TEST_DIR)/$(TEST_CONF_FILE)

# Include unit test rules
include tests.mk
