NAME     := webserv

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g3

SRC      := srcs/main.cpp srcs/Server.cpp
OBJDIR   := obj
OBJ      := $(SRC:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Compilation complete. Executable created: $(NAME)"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJDIR)
	@echo "Object files removed."

fclean: clean
	@rm -f $(NAME)
	@echo "Cleaned up: $(NAME)"

re: fclean all

.PHONY: all clean fclean re