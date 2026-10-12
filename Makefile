C++ = c++
CCFLAGS = -Wall -Wextra -Werror
NAME = ircserv
SRCS = srcs/main.c
OBJECTS = $(SRCS:.c=.o)

COLOR_GREEN=\033[38;5;46m
COLOR_RED=\033[38;2;255;0;0m
COLOR_BLUE=\033[38;2;0;157;255m
END_COLOR=\033[0m

all: $(NAME)	

$(NAME): $(OBJECTS)
	@$(C++) $(CCFLAGS) $(OBJECTS) -o $(NAME)
	@echo "\n$(COLOR_GREEN)COMPILATION SUCCESS $(END_COLOR)"

%.o: %.c
	@$(CC) $(CCFLAGS) -c $< -o $@
	@echo "$(COLOR_BLUE)[COMPILED] $(END_COLOR)$<"

clean:
	@rm -f $(OBJECTS)
	@$(foreach file,$(OBJECTS),echo "$(COLOR_RED)[DELETED]$(END_COLOR) $(file)";)

fclean: clean
	@rm -f $(NAME)
	@echo "$(COLOR_RED)[DELETED]$(END_COLOR) $(NAME)"
	@echo "\n$(COLOR_GREEN)FCLEAN SUCCESS$(END_COLOR)"

re: fclean all