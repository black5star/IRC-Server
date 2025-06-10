NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror  -std=c++98
SRC = main.cpp Server.cpp
OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(OBJ) $(CXXFLAGS) -o $(NAME)
	@echo "executable is ready"

clean:
	@rm -f $(OBJ)
	@echo "object removed"

fclean:
	@rm -f $(OBJ) $(NAME)
	@echo "object and executable removed"

re: fclean all

.PHONY: all clean fclean re
