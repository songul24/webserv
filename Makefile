NAME = webserv


# Use find or add the prefix so make knows to look in the 'src' directory
SRCS = $(addprefix src/, $(shell ls src | grep .cpp))
OBJS = $(SRCS:.cpp=.o)

CXX = c++
CXX_FLAGS = -Wall -Wextra -Werror -std=c++98 #-g3 -fsanitize=address -fsanitize=leak

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




# NAME= webserv
# FLAGS= -std=c++98 -Wall -Wextra -Werror -g3 #-fsanitize=address -fsanitize=leak
# SRCS= src/main.cpp src/Cgi.cpp src/Configfile.cpp src/Connection.cpp src/Delete_method.cpp \
# 	src/Get_methode.cpp src/Post_method.cpp scr/Request_header.cpp src/Request_line.cpp \
# 	src/Request.cpp src/Server.cpp src/utils.cpp src/Webserver.cpp
# OBJS=$(SRCS:.cpp=.o)

# all: $(NAME)

# %.o : %.cpp
# 	c++ $(FLAGS) -c $^ -o $@

# $(NAME): $(OBJS)
# 	c++ $(FLAGS) $^ -o $@

# clean:
# 	rm -rf $(OBJS)

# fclean: clean
# 	rm -rf $(NAME)

# re : fclean all

# run : re clean
# 	./$(NAME) $(ARGS)

# debug : re
# 	valgrind --leak-check=full --track-fds=yes ./$(NAME) $(ARGS)

# push : fclean
# 	git add .
# 	git commit -m "$(ARGS)"
# 	git push #class

# .PHONY: all clean fclean re run debug push
# .SECONDARY: $(OBJS)