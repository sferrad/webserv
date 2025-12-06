NAME     := webserv

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g3

SRC      := srcs/main.cpp srcs/Server.cpp srcs/HttpRequestHandler.cpp srcs/utils/utils_parsing.cpp srcs/conf/ServerConf.cpp srcs/CGI/HandleCGI.cpp
OBJDIR   := obj
OBJ      := $(SRC:%.cpp=$(OBJDIR)/%.o)

# Couleurs simples
RED    := \033[1;31m
GREEN  := \033[1;32m
YELLOW := \033[1;33m
BLUE   := \033[1;34m
CYAN   := \033[1;36m
RESET  := \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(CYAN)[Webserv] Starting compilation...$(RESET)"
	@for f in $(OBJ); do \
		echo -e "$(YELLOW)• Compiling $$f$(RESET)"; \
		sleep 0.05; \
	done
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "$(GREEN)[Webserv] ✅ Compilation complete!$(RESET)"
	@echo "$(BLUE)[Webserv] Ready to serve 🚀$(RESET)"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJDIR)
	@echo "$(RED)[Webserv] 🧹 Object files removed.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)[Webserv] 🧽 Executable removed.$(RESET)"

re: fclean all

.PHONY: all clean fclean re
