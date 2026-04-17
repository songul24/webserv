NAME = webserv

# Use find or add the prefix so make knows to look in the 'src' directory
SRCS = $(addprefix src/, $(shell ls src | grep .cpp))
OBJS = $(SRCS:.cpp=.o)

CXX = c++
CXX_FLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(OBJS) -o $(NAME)

# This rule now correctly maps src/%.o to src/%.cpp
src/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re