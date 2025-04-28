# WizuAll Compiler

This project implements a compiler for WizuAll, a simple procedural language designed for basic data manipulation and visualization generation via C code and external tools like gnuplot.

## Features

*   Parses WizuAll programs (.wzu files).
*   Performs basic semantic checks (e.g., variable declaration).
*   Generates intermediate C code (`output.c`).
*   Supports scalar (double) variables.
*   Supports vector variables (loaded from files).
*   Arithmetic operations: +, -, *, /, unary -
*   Control flow: `if`/`else`, `while` loops.
*   Assignments.
*   Built-in functions for data loading and visualization:
    *   `load_vector("datafile.dat", column_index)`: Loads a column into a vector variable (assigned via `=`). **Note:** requires datafile name as an ID currently, string literals not yet supported.
    *   `print_vector(vector_variable)`: Prints vector contents.
    *   `average(vector_variable)`: Computes and prints the average.
    *   `max_val(vector_variable)`: Computes and prints the maximum value.
    *   `plot_xy(x_vector, y_vector)`: Creates a simple line/point plot using gnuplot.
    *   `save_plot("filename.png")`: TODO: Intended to save the next plot to a file.
    *   `histogram(vector_variable)`: TODO: Intended to generate a histogram.

## Language Syntax (Informal)

```wizuall
// Example WizuAll Program (e.g., example.wzu)

// Comments start with //

// Variable assignment (scalars are doubles)
count = 0;
limit = 10.5;
step = 1;

// Load data (assuming datafile has space/tab/comma separated columns)
// Creates a vector variable `myData`
// NOTE: Current implementation expects datafile name without quotes.
dataFileId = "my_data.dat"; // Need a way to represent strings or use ID directly
myData = load_vector(dataFileId, 0); // Load column 0

// While loop
while (count < limit) {
    temp = count * 2.1 - 5;
    
    // If/Else statement
    if (temp > 0) {
        result = temp;
    } else {
        result = -temp;
    }
    
    count = count + step;
}

// Function calls for stats/output
print_vector(myData);
average(myData);
max_val(myData);

// Plotting (assuming another vector 'indices' exists or is created)
// indices = create_indices(size(myData)); // Hypothetical function
// plot_xy(indices, myData);

final_result = result; // Final assignment

// Empty statement
;
```

## Build Instructions

Requires `flex`, `bison`, and `gcc` (or compatible C compiler).

1.  Navigate to the project root directory.
2.  Run `make clean` to remove previous builds.
3.  Run `make` to build the `wizuallc` compiler executable.

**Note:** If you encounter errors during the `flex` step, please ensure `flex` is correctly installed and functioning in your environment.

## Running the Compiler

```bash
./wizuallc [input_program.wzu] [output_c_file.c]
```

*   `input_program.wzu`: Path to your WizuAll source file. If omitted, reads from standard input.
*   `output_c_file.c`: Optional path for the generated C code. Defaults to `output.c`.

Example:
```bash
./wizuallc examples/my_program.wzu my_output.c
```

## Compiling and Running Generated C Code

1.  Compile the generated C code using `gcc`:
    ```bash
    gcc -Wall my_output.c -o my_program_exe -lm 
    ```
    (The `-lm` is needed if using math functions like `INFINITY`).

2.  Run the compiled C executable:
    ```bash
    ./my_program_exe
    ```
    Ensure any data files needed by `load_vector` are present in the same directory or provide correct paths.
    Ensure `gnuplot` is installed and in your PATH for plotting functions to work.

## Project Structure

*   `src/`: Source files (.l, .y, .c)
*   `include/`: Header files (.h)
*   `build/`: Intermediate build files (.o, generated .c, .h)
*   `examples/`: Sample WizuAll programs and data files.
*   `Makefile`: Build configuration.
*   `README.md`: This file.
*   `wizuallc`: Compiled compiler executable.
*   `output.c`: Default output file for generated C code.

## Limitations and Future Work

*   **Flex Issues:** The build process may fail due to persistent issues with `flex` execution in some environments.
*   **String Literals:** Not fully supported (e.g., for filenames).
*   **Vector Operations:** Limited support (loading, printing, basic stats). Arithmetic vector ops not implemented.
*   **Type System:** Very basic. Mostly assumes `double`. Limited compile-time type checking.
*   **Error Handling:** Semantic error detection is basic and primarily within code generation.
*   **Gnuplot Integration:** `save_plot` and `histogram` are placeholders.
*   **Scope:** Only global scope is implemented.
*   **Functions:** User-defined functions are not supported. 