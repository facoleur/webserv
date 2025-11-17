# Unit tests
UNIT_TEST_DIR := tests/unitTests
UNIT_TEST_BIN_DIR := $(UNIT_TEST_DIR)/bin
UNIT_TEST_SRCS := $(wildcard $(UNIT_TEST_DIR)/*.cpp)
UNIT_TEST_BINS := $(patsubst $(UNIT_TEST_DIR)/%.cpp,$(UNIT_TEST_BIN_DIR)/%,$(UNIT_TEST_SRCS))

SRC_FILES := $(filter-out $(SRC_DIR)/main.cpp,$(wildcard $(SRC_DIR)/*.cpp))

TEST_CC := g++
TEST_FLAGS := -Wall -Wextra -Werror -std=c++98 

tests: $(UNIT_TEST_BINS)
	@echo "$(GREEN)Running all unit tests...$(DEF_COLOR)"
	@for test in $(UNIT_TEST_BINS); do \
		echo "$(YELLOW)Running $$test...$(DEF_COLOR)"; \
		$$test || exit 1; \
	done
	@echo "$(GREEN)All tests completed successfully.$(DEF_COLOR)"

$(UNIT_TEST_BIN_DIR)/%: $(UNIT_TEST_DIR)/%.cpp $(SRC_FILES)
	@mkdir -p $(UNIT_TEST_BIN_DIR)
	@echo "$(YELLOW)Building test $@...$(DEF_COLOR)"
	$(TEST_CC) $(TEST_FLAGS) -I$(INC_DIR) $(SRC_FILES) $< -o $@

clean-tests:
	@echo "$(BLUE)Cleaning unit test binaries...$(DEF_COLOR)"
	@$(RM) $(UNIT_TEST_BIN_DIR)
