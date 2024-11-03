NAME			:= whid
CC				:= gcc
SRC_DIR			:= ./src
BUILD_DIR		:= ./build
BIN_DIR			:= ./bin
SRC_FILES		:= $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES		:= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
CFLAGS			:= -Wall
CLIBS			:= $(shell command xml2-config --cflags --libs)
CLANG-FORMAT	:= $(shell command -v clang-format 2> /dev/null)
CLANG-TIDY		:= $(shell command -v clang-tidy 2> /dev/null)

all: clean config style-format style-tidy release

# debug
DBG_BIN_DIR		:= $(BIN_DIR)/debug
DBG_BUILD_DIR	:= $(BUILD_DIR)/debug
DBG_DIRS		:= $(DBG_BUILD_DIR) $(DBG_BIN_DIR)
DBG_FLAGS		:= -g -O0 -v -da -Q
DBG_EXE			:= $(NAME)_debug
DBG_OBJ_FILES	:= $(patsubst $(SRC_DIR)/%.c,$(DBG_BUILD_DIR)/%.o,$(SRC_FILES))

debug: $(DBG_EXE)

$(DBG_EXE): $(DBG_OBJ_FILES)
	@echo "Linking $(DBG_EXE):"
	$(CC) -o $(DBG_BIN_DIR)/$@ $^

$(DBG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling object $@"
	$(CC) -c $(CFLAGS) $(CLIBS) $(DBG_FLAGS) -o $@ $<

# release
REL_BIN_DIR		:= $(BIN_DIR)/release
REL_BUILD_DIR	:= $(BUILD_DIR)/release
REL_DIRS		:= $(REL_BUILD_DIR) $(REL_BIN_DIR)
REL_FLAGS		:= -O2
REL_OBJ_FILES	:= $(patsubst $(SRC_DIR)/%.c,$(REL_BUILD_DIR)/%.o,$(SRC_FILES))

release: $(NAME)

$(NAME): $(REL_OBJ_FILES)
	@echo "Linking $(NAME):"
	$(CC) -o $(REL_BIN_DIR)/$@ $^

$(REL_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling object $@"
	$(CC) -c $(CFLAGS) $(CLIBS) $(REL_FLAGS) -o $@ $<

style-format:
ifneq ($(CLANG-FORMAT),)
	@echo "Clang-format formatting:"
	@for src in $(SRC_FILES) ; do \
		echo "Formatting $$src..."; \
		clang-format -i "$$src" ; \
	done
else
	@echo "Warning: clang-format>=3.8.0 is not installed...Skipping"
endif

style-tidy:
ifneq ($(CLANG-TIDY),)
	@echo "Clang-tiddy formatting:"
	@for src in $(SRC_FILES) ; do \
		echo "Formatting $$src..."; \
		clang-tidy -fix "$$src" --; \
	done
else
	@echo "Warning: clang-tidy>=3.8.0 is not installed...Skipping"
endif

# config pre build
config:
	@echo create directories
	$(info $(shell mkdir -p $(REL_DIRS) $(DBG_DIRS)))

.PHONY: clean

clean:
	@echo "Cleaning:"
	clear
	$(RM) $(BUILD_DIR)/release/*
	$(RM) $(BUILD_DIR)/debug/*
	$(RM) $(BIN_DIR)/release/*
	$(RM) $(BIN_DIR)/debug/*