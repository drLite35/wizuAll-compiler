# Compiler and flags
CC = gcc
CFLAGS = -Wall -Iinclude -g
# Add BUILD_DIR to CFLAGS for generated files
CFLAGS_GEN = $(CFLAGS) -I$(BUILD_DIR)
LDFLAGS = 

# Flex and Bison
FLEX = flex -L --posix
BISON = bison -d

# Directories
SRC_DIR = src
BUILD_DIR = build
INC_DIR = include

# Source files
C_SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/ast.c $(SRC_DIR)/symtab.c $(SRC_DIR)/codegen.c $(BUILD_DIR)/lex.yy.c $(BUILD_DIR)/parser.tab.c
LEX_SRC = $(SRC_DIR)/lexer.l
PARSER_SRC = $(SRC_DIR)/parser.y

# Generated files
LEX_GEN = $(BUILD_DIR)/lex.yy.c
PARSER_GEN_C = $(BUILD_DIR)/parser.tab.c
PARSER_GEN_H = $(BUILD_DIR)/parser.tab.h

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter %main.c %ast.c %symtab.c %codegen.c, $(C_SOURCES)))
OBJECTS += $(patsubst $(BUILD_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter %lex.yy.c %parser.tab.c, $(C_SOURCES)))

# Executable name
TARGET = wizuallc

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS)

# Compile C source files (including ast.c, symtab.c)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h # Removed dependency on parser.tab.h for non-generated files
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile generated C source files
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c $(PARSER_GEN_H) $(INC_DIR)/*.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_GEN) -c $< -o $@

# Generate parser files from Bison
$(PARSER_GEN_C) $(PARSER_GEN_H): $(PARSER_SRC)
	@mkdir -p $(@D)
	$(BISON) $(PARSER_SRC) -o $(PARSER_GEN_C) --defines=$(PARSER_GEN_H)

# Generate lexer file from Flex
$(LEX_GEN): $(LEX_SRC) $(PARSER_GEN_H) # Depends on parser header for token definitions
	@mkdir -p $(@D)
	$(FLEX) -o $@ $<

# Clean up build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) lex.yy.c parser.tab.c parser.tab.h

# Phony targets
.PHONY: all clean 