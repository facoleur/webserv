FLAGS= -g # -Wall -Wextra -Werror -std=c++98

SRCDIR= src/
SRCS= main.cpp Server.cpp Config.cpp

OBJS = $(addprefix $(SRCDIR), $(SRCS:.cpp=.o))
NAME= webserv

all: $(NAME)

$(NAME): $(OBJS)
	c++ $(FLAGS) -o $(NAME) $(OBJS)

$(SRCDIR)%.o: $(SRCDIR)%.cpp
	c++ $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
