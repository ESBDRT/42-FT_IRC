C++ = c++
CCFLAGS = -Wall -Wextra -Werror -std=c++98
NAME = ircserv
SRCS = srcs/main.cpp srcs/errors/errors.cpp
OBJECTS = $(SRCS:.cpp=.o)

CLR_GREEN=\033[38;5;46m
CLR_RED=\033[38;2;255;0;0m
CLR_BLUE=\033[38;2;0;157;255m
CLR_RST=\033[0m

all: $(NAME)	

$(NAME): $(OBJECTS)
	@$(C++) $(CCFLAGS) $(OBJECTS) -o $(NAME)
	@echo "\n$(CLR_GREEN)$(words $(OBJECTS)) $(CLR_RST)FILES COMPILED"
	@echo "$(CLR_GREEN)[PROJECT $(NAME) COMPILATION SUCCESS]$(CLR_RST)"

%.o: %.cpp
	@$(CC) $(CCFLAGS) -c $< -o $@
	@echo "$(CLR_BLUE)[COMPILED] $(CLR_RST)$<"

clean:
	@rm -f $(OBJECTS)
	@$(foreach file,$(OBJECTS),echo "$(CLR_RED)[DELETED]$(CLR_RST) $(file)";)

fclean: clean
	@rm -f $(NAME)
	@echo "$(CLR_RED)[DELETED]$(CLR_RST) $(NAME)"
	@echo "\n$(CLR_GREEN)$(words $(OBJECTS)) $(CLR_RST)FILES DELETED"
	@echo "$(CLR_GREEN)[PROJECT $(NAME) FCLEAN SUCCESS]$(CLR_RST)"

re: fclean all