# WizuAll Compiler

## Overview

This project implements a compiler for **WizuAll**, a simple, C-like procedural language focused on numerical computation, basic data handling (scalars and vectors), and generating visualizations via external tools, primarily `gnuplot`.

The compiler (`wizuallc`) reads a WizuAll program (`.wzu` file), parses it, performs basic semantic checks, builds an Abstract Syntax Tree (AST), and generates equivalent C code (`.c` file). This generated C code can then be compiled using a standard C compiler (like `gcc`) and executed to perform the computations and produce visualizations.

## Features

*   **Parsing:** Parses WizuAll programs using `flex` (lexer) and `bison` (parser).
*   **AST:** Constructs an Abstract Syntax Tree representing the program structure.
*   **Symbol Table:** Basic symbol table tracks variable names and types (scalar/vector).
*   **Code Generation:** Generates C code from the AST.
*   **Data Types:** Supports `double` floating-point scalars and `Vector` (dynamic array of doubles).
*   **Arithmetic:** Standard operators (`+`, `-`, `*`, `/`) and unary minus (`-`).
*   **Control Flow:** `if`/`else` conditional statements and `while` loops.
*   **Assignments:** Assigning values to variables (`var = expression;`).
*   **Data Loading:** Built-in function `load_vector` to load numerical data from columns in text files.
*   **Visualization & Output (Built-ins):**
    *   `print_vector(vec)`: Prints vector contents to standard output.
    *   `average(vec)`: Calculates and prints the average of a vector.
    *   `max_val(vec)`: Calculates and prints the maximum value in a vector.
    *   `plot_xy(x_vec, y_vec)`: Generates a 2D line/point plot using `gnuplot`.
    *   `save_plot(filename)`: (Planned) Saves the subsequent plot to a file.
    *   `histogram(vec)`: (Planned) Generates a histogram plot using `gnuplot`.

## WizuAll Language Specification

### General Structure

A WizuAll program is a sequence of statements. Execution proceeds sequentially, modified by control flow statements.

```wizuall
// Single-line comments start with //

statement1;
statement2;

// Block of statements
{
    statement3;
    statement4;
}

statement5;
```

### Data Types

*   **Scalar:** Represented as `double` in the generated C code. Supports standard floating-point literals (e.g., `10`, `3.14`, `-0.5`).
*   **Vector:** A dynamic array of doubles. Represented by a `Vector` struct in C (`{ double* data; size_t size; }`). Vectors are primarily created via `load_vector`.

### Variables and Assignment

*   Variables are declared implicitly upon first assignment.
*   Variable names (Identifiers) start with a letter or underscore, followed by letters, numbers, or underscores (`[a-zA-Z_][a-zA-Z0-9_]*`).
*   Assignment uses the `=` operator. The type of the variable is determined by the first assignment (currently loosely enforced, especially with `load_vector` setting type to `VECTOR`).
    ```wizuall
    my_scalar = 10.5 * 2;
    my_vector = load_vector(file_id, 0);
    ```

### Operators and Expressions

Standard arithmetic operators are supported:

| Operator | Description        | Associativity | Precedence (Higher binds tighter) |
| :------- | :----------------- | :------------ | :-------------------------------- |
| `-`      | Unary Minus        | Right         | Highest                           |
| `*`, `/` | Multiplication, Division | Left          | Medium                            |
| `+`, `-` | Addition, Subtraction | Left          | Medium                            |
| `=`      | Assignment         | Right         | Lowest                            |

Parentheses `()` can be used to override precedence.

*   **Type Compatibility:** Currently, arithmetic operations (`+`, `-`, `*`, `/`) are generated assuming scalar (`double`) operands. Operations involving vectors are not yet supported (e.g., `vector + scalar`, `vector * vector`). This is a major limitation.

### Control Flow

*   **If-Else Statement:**
    ```wizuall
    if (condition_expression) {
        // statements if true
    }
    
    if (value > threshold) {
        result = 1;
    } else {
        result = 0;
    }
    ```
*   **While Loop:**
    ```wizuall
    while (condition_expression) {
        // loop body statements
        counter = counter + 1;
    }
    ```

### Blocks

Statements can be grouped into blocks using curly braces `{}`. Blocks are required for the bodies of `if`, `else`, and `while`.

### Built-in Functions

Built-in functions are called using standard function call syntax `function_name(arg1, arg2, ...)`. They are treated as statements unless specified otherwise.

*   `load_vector(filename_id, column_index)`:
    *   **Purpose:** Reads numerical data from a text file into a vector.
    *   **Arguments:**
        *   `filename_id`: An *identifier* whose associated value (currently assigned directly, e.g., `fid = "data.txt"`) holds the filename string. **Limitation:** Direct string literals are not yet supported here.
        *   `column_index`: A scalar expression evaluating to the 0-based index of the column to read.
    *   **Behavior:** This function must be used on the right-hand side of an assignment (`my_vec = load_vector(...)`). It generates C code that:
        1.  Counts the rows in the specified file.
        2.  Allocates memory for the target vector based on the row count.
        3.  Reads the specified column, converting values to doubles, and populates the vector.
        4.  Updates the symbol table entry for the assigned variable to `TYPE_VECTOR`.
    *   **Data File Format:** Assumes columns are separated by spaces, tabs, or commas.

*   `print_vector(vector_id)`:
    *   **Purpose:** Prints the contents of a vector variable to standard output.
    *   **Arguments:** `vector_id`: The identifier of the vector variable.
    *   **Behavior:** Generates C code to loop through the vector's data and print it in a readable format.

*   `average(vector_id)`:
    *   **Purpose:** Calculates and prints the mean average of the elements in a vector.
    *   **Arguments:** `vector_id`: The identifier of the vector variable.
    *   **Behavior:** Generates C code to compute the sum and divide by the size, then prints the result.

*   `max_val(vector_id)`:
    *   **Purpose:** Finds and prints the maximum value within a vector.
    *   **Arguments:** `vector_id`: The identifier of the vector variable.
    *   **Behavior:** Generates C code to iterate through the vector and find the maximum element, then prints it.

*   `plot_xy(x_vector_id, y_vector_id)`:
    *   **Purpose:** Creates a 2D plot using `gnuplot`.
    *   **Arguments:**
        *   `x_vector_id`: Identifier of the vector for the X-axis.
        *   `y_vector_id`: Identifier of the vector for the Y-axis.
    *   **Behavior:** Generates C code that:
        1.  Checks if X and Y vectors have the same size.
        2.  Writes the corresponding X, Y pairs to a temporary file (`gnuplot_data.tmp`).
        3.  Opens a pipe to `gnuplot` using `popen()`.
        4.  Sends a `plot` command to gnuplot to display the data from the temporary file.
        5.  Closes the pipe and removes the temporary file.
    *   **Requires:** `gnuplot` must be installed and accessible in the system's PATH for the generated C code to work.

*   `save_plot(filename_id)`: **(Placeholder)**
    *   **Purpose:** Intended to configure the *next* plot generated by `plot_xy` (or other plotting functions) to be saved to the specified file instead of displayed on screen.
    *   **Arguments:** `filename_id`: Identifier associated with the output filename.
    *   **Current Status:** Generates only a placeholder comment. Requires significant changes to code generation state management or strategy (e.g., generating a separate gnuplot script).

*   `histogram(vector_id)`: **(Placeholder)**
    *   **Purpose:** Intended to generate a histogram of the data in the vector using `gnuplot`.
    *   **Arguments:** `vector_id`: Identifier of the vector.
    *   **Current Status:** Generates only a placeholder comment. Requires implementing C-side data binning and specific gnuplot commands.

### Grammar (BNF-like, derived from parser.y)

```
program         : statement_list

statement_list  : /* empty */
                | statement_list statement

statement       : assignment_statement
                | expression_statement
                | if_statement
                | while_statement
                | block

assignment_statement : ID '=' expr ';'

expression_statement : expr ';'
                     | ';'

if_statement    : T_IF '(' expr ')' block 
                | T_IF '(' expr ')' block T_ELSE block

while_statement : T_WHILE '(' expr ')' block

block           : '{' statement_list '}'

expr            : expr '+' term
                | expr '-' term
                | term

term            : term '*' factor
                | term '/' factor
                | factor

factor          : '(' expr ')'
                | '-' factor /* Unary Minus */
                | NUM          /* Number Literal */
                | ID           /* Identifier */
                | vector_literal
                | ID '(' optional_arg_list ')' /* Function Call */

vector_literal  : '[' optional_expr_list ']'

optional_expr_list : /* empty */
                   | expr_list

expr_list       : expr
                | expr_list ',' expr

optional_arg_list : /* empty */
                  | arg_list

arg_list        : expr
                | arg_list ',' expr 

```
*Note: Operator precedence and associativity are handled by `%left`, `%right`, and `%prec` directives in `parser.y`, not explicitly shown in these rules.* 
*Note: `T_IF`, `T_ELSE`, `T_WHILE`, `NUM`, `ID` are tokens returned by the lexer.* 

## Build Instructions

Requires `flex`, `bison`, and `gcc` (or a compatible C compiler).

1.  Clone the repository (if applicable).
2.  Navigate to the project root directory.
3.  Run `make clean` to remove previous build artifacts.
4.  Run `make` to build the `wizuallc` compiler executable.

**Build Troubleshooting:**
*   **Flex Errors:** If you encounter errors like `unrecognized rule` during the `flex` step, it often indicates an issue with your `flex` installation, environment variables, file encoding, or potentially hidden characters in `src/lexer.l`. Verify your installation (`flex --version`), try reinstalling (`brew reinstall flex`, `sudo apt-get install flex`, etc.), and ensure `.l` files are plain text (ASCII/UTF-8).
*   **Bison Errors:** Usually related to grammar ambiguities (shift/reduce, reduce/reduce conflicts) or syntax errors in `src/parser.y`.
*   **GCC Errors:** Standard C compilation errors in your `.c` files or the files generated by flex/bison.

## Running the Compiler

```bash
./wizuallc [input_program.wzu] [output_c_file.c]
```

*   `input_program.wzu`: (Optional) Path to your WizuAll source file. If omitted, the compiler reads from standard input (end input with Ctrl+D/Ctrl+Z).
*   `output_c_file.c`: (Optional) Path for the generated C code. Defaults to `output.c` in the current directory.

**Examples:**

*   Compile `my_prog.wzu` and generate `my_prog.c`:
    ```bash
    ./wizuallc examples/my_prog.wzu my_prog.c
    ```
*   Compile from standard input, generate `output.c`:
    ```bash
    ./wizuallc 
    # Type your WizuAll code here...
    # Press Ctrl+D when finished
    ```

## Compiling and Running Generated C Code

The WizuAll compiler *generates* C code; it doesn't execute the program directly.

1.  **Compile the Generated C Code:** Use a C compiler (like `gcc`). You will likely need the math library (`-lm`).
    ```bash
    gcc -Wall output.c -o program_executable -lm 
    ```
    Replace `output.c` with your generated C filename and `program_executable` with your desired output name.

2.  **Run the Compiled Executable:**
    ```bash
    ./program_executable
    ```

**Runtime Requirements:**
*   Any data files referenced by `load_vector` must exist and be accessible when the *C executable* is run.
*   `gnuplot` must be installed and in the system's PATH for plotting functions (`plot_xy`) to work when the *C executable* is run.

## Project Structure

```
. 
├── Makefile           # Build configuration
├── README.md          # This documentation
├── build/             # Intermediate build files (.o, generated .c/.h)
├── examples/          # Sample WizuAll code and data
│   ├── control_flow.wzu
│   ├── data.dat
│   ├── dummy.dat
│   ├── math_example.wzu
│   ├── plotting_prep.wzu
│   ├── simple_calc.wzu
│   └── vector_test.wzu
├── include/           # Header files (.h)
│   ├── ast.h
│   ├── codegen.h
│   ├── symtab.h
│   └── wizuall.h      # Currently unused placeholder
├── src/               # Source files (.l, .y, .c)
│   ├── ast.c
│   ├── codegen.c
│   ├── lexer.l
│   ├── main.c
│   ├── parser.y
│   └── symtab.c
└── wizuallc           # Compiler executable (after running make)
```

## Code Generation Strategy

The compiler generates a standalone C program containing:

1.  **Includes:** Necessary standard C headers (`stdio.h`, `stdlib.h`, `string.h`, `math.h`).
2.  **Data Structures:** A `struct Vector` definition.
3.  **Runtime Helpers:** Static C functions for operations needed by built-ins (e.g., `count_file_rows`, `read_double_column`, `print_vector_runtime`, `average_runtime`, `max_val_runtime`, gnuplot helpers).
4.  **`main()` Function:**
    *   **Variable Declarations:** Declares all variables identified during parsing (from the symbol table) as `double` or `Vector`, initialized to default values.
    *   **Code Body:** Translates the WizuAll statement list into corresponding C statements, function calls, loops, and conditionals.
    *   **Cleanup:** Includes code to free dynamically allocated memory (currently just for Vectors).

## Error Handling

*   **Lexical Errors:** Invalid characters are reported by the lexer (`lexer.l`) with line numbers.
*   **Syntax Errors:** Parsing errors (incorrect grammar) are reported by `yyerror` in `parser.y` with line numbers.
*   **Semantic Errors:** Basic checks are performed during code generation (`codegen.c`):
    *   Use of undeclared identifiers.
    *   Assignment to undeclared identifiers (should ideally be caught earlier).
    *   Placeholder comments/errors for unimplemented features or incorrect built-in usage.
*   **Runtime Errors:** Errors during the execution of the *generated C code* (e.g., file not found by `load_vector`, `gnuplot` not found, division by zero) are handled by the generated C code's logic or standard C runtime behaviour.

## Limitations and Future Work

*   **Flex Environment:** The build process is known to be sensitive to the `flex` installation/environment.
*   **String Literals:** Only identifiers are currently supported for filenames in `load_vector` and `save_plot`. Proper string literal support is needed.
*   **Vector Implementation:** 
    *   Vector creation only via `load_vector`.
    *   No element access (e.g., `my_vec[i]`).
    *   No vector arithmetic (e.g., `v1 + v2`, `scalar * v1`).
*   **Type System/Checking:** Very rudimentary. Assumes `double` for most things. No compile-time checks for vector/scalar mismatches in operations.
*   **Code Generation for Built-ins:** `save_plot` and `histogram` are incomplete. `load_vector` expects assignment. Functions used in expressions need return value handling.
*   **Scope:** Only a single, global scope is implemented.
*   **User-defined Functions:** Not supported.
*   **Error Reporting:** Semantic error messages could be more specific and occur earlier (e.g., during a dedicated semantic analysis phase after parsing).


</rewritten_file> 