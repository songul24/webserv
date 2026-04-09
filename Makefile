NAME = webserv

SRCS = src/main.cpp src/WebServer.cpp src/Connection.cpp src/Server.cpp \

OBJS = $(SRCS:.cpp=.o)

CXX = c++
CXX_FLAGS = -Wall -Wextra -Werror -std=c++98

all:$(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
.SECONDARY:$(OBJS)
.PHONY: all clean fclean re