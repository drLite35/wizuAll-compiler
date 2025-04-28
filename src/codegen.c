#include "codegen.h"
#include <stdio.h>
#include <stdlib.h> // For exit
#include <string.h> // For strcat and strcpy

// Forward declaration for the recursive expression generator
static void generateExpressionCode(Node* node, FILE* outfile);

// Helper to generate C code for a single statement or expression
static void generateStatementCode(Node* node, FILE* outfile, int indentLevel) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < indentLevel; ++i) {
        fprintf(outfile, "    "); // 4 spaces per indent level
    }

    switch (node->type) {
        case NODE_ASSIGN:
            // Check LHS exists (it should have been inserted by parser)
            Symbol* lhs_sym = symtab_lookup(node->data.assignOp.name);
            if (!lhs_sym) {
                 fprintf(stderr, "Codegen Error line %d: Assignment to undeclared identifier '%s' (Internal Error?)\n", node->lineno, node->data.assignOp.name);
                 // Don't generate code for this broken assignment
                 break; 
            }
            // Check if RHS is a load_vector call
            if (node->data.assignOp.value && node->data.assignOp.value->type == NODE_FUNC_CALL && 
                strcmp(node->data.assignOp.value->data.funcCall.name, "load_vector") == 0) 
            {
                // Generate code for: Vector var = load_vector(filename, col);
                Node* funcNode = node->data.assignOp.value;
                Node* filename_arg = funcNode->data.funcCall.args; 
                Node* column_arg = filename_arg ? filename_arg->next : NULL;

                if (filename_arg && filename_arg->type == NODE_ID /* Allow string literals later */ && 
                    column_arg && column_arg->type == NODE_NUM && !column_arg->next) 
                {   
                    // **Update symbol table type!** (Crucial step missed earlier)
                    // Ideally done in semantic analysis, but do it here for now.
                    Symbol* sym = symtab_lookup(node->data.assignOp.name);
                    if (sym) sym->type = TYPE_VECTOR;
                     else { /* Error: Assigning to undeclared? Should be caught earlier */ }

                    char* filename_str = filename_arg->data.id.sval; // Use ID name as filename
                    int column_idx = (int)column_arg->data.dval;

                    // 1. Get row count
                    fprintf(outfile, "size_t %s_rows = count_file_rows(\"%s\");\n", node->data.assignOp.name, filename_str);
                    // 2. Allocate vector (using generated helper)
                    fprintf(outfile, "%s%s = create_vector(%s_rows);\n", indentStr, node->data.assignOp.name, node->data.assignOp.name);
                    // 3. Read data (using generated helper)
                    fprintf(outfile, "%sif (%s_rows > 0) {\n", indentStr, node->data.assignOp.name);
                    fprintf(outfile, "%s    read_double_column(\"%s\", %d, %s.data, %s.size);\n", indentStr, filename_str, column_idx, node->data.assignOp.name, node->data.assignOp.name);
                    fprintf(outfile, "%s}\n", indentStr);

                } else {
                    fprintf(outfile, "/* Codegen Error: Invalid arguments for load_vector assignment on line %d */\n", node->lineno);
                }

            } else { // Normal assignment
                fprintf(outfile, "%s = ", node->data.assignOp.name);
                generateExpressionCode(node->data.assignOp.value, outfile);
                fprintf(outfile, ";\n");
            }
            break; // End of NODE_ASSIGN
        case NODE_NUM: case NODE_ID: case NODE_BINOP: case NODE_UNARYOP: case NODE_VEC:
             generateExpressionCode(node, outfile);
             fprintf(outfile, ";\n");
             break;
        case NODE_IF:
            fprintf(outfile, "if (");
            generateExpressionCode(node->data.ifStmt.condition, outfile);
            fprintf(outfile, ") {\n");
            Node* thenStmt = node->data.ifStmt.then_branch;
            while (thenStmt) {
                generateStatementCode(thenStmt, outfile, indentLevel + 1);
                thenStmt = thenStmt->next;
            }
            for (int i = 0; i < indentLevel; ++i) fprintf(outfile, "    "); // Indent closing brace
            fprintf(outfile, "}");
            if (node->data.ifStmt.else_branch) {
                fprintf(outfile, " else {\n");
                Node* elseStmt = node->data.ifStmt.else_branch;
                while (elseStmt) {
                    generateStatementCode(elseStmt, outfile, indentLevel + 1);
                    elseStmt = elseStmt->next;
                }
                for (int i = 0; i < indentLevel; ++i) fprintf(outfile, "    "); // Indent closing brace
                fprintf(outfile, "}\n");
            } else {
                fprintf(outfile, "\n");
            }
            break;
        case NODE_WHILE:
            fprintf(outfile, "while (");
            generateExpressionCode(node->data.whileStmt.condition, outfile);
            fprintf(outfile, ") {\n");
            Node* bodyStmt = node->data.whileStmt.body;
            while (bodyStmt) {
                generateStatementCode(bodyStmt, outfile, indentLevel + 1);
                bodyStmt = bodyStmt->next;
            }
            for (int i = 0; i < indentLevel; ++i) fprintf(outfile, "    "); // Indent closing brace
            fprintf(outfile, "}\n");
            break;
        case NODE_FUNC_CALL:
            // Visualization and other built-ins
            if (strcmp(node->data.funcCall.name, "print_vector") == 0) {
                Node* vec_arg = node->data.funcCall.args;
                if (vec_arg && vec_arg->type == NODE_ID && !vec_arg->next) {
                    fprintf(outfile, "print_vector_runtime(%s, \"%s\");\n", 
                           vec_arg->data.id.sval, vec_arg->data.id.sval);
                } else {
                    fprintf(outfile, "/* Codegen Error: Invalid arguments for print_vector */\n");
                }
            } else if (strcmp(node->data.funcCall.name, "average") == 0) {
                Node* vec_arg = node->data.funcCall.args;
                if (vec_arg && vec_arg->type == NODE_ID && !vec_arg->next) {
                     fprintf(outfile, "printf(\"Average of %s: %s\\n\", average_runtime(%s));\n", 
                            vec_arg->data.id.sval, "%f", vec_arg->data.id.sval);
                } else {
                    fprintf(outfile, "/* Codegen Error: Invalid arguments for average */\n");
                }
            } else if (strcmp(node->data.funcCall.name, "max_val") == 0) {
                 Node* vec_arg = node->data.funcCall.args;
                 if (vec_arg && vec_arg->type == NODE_ID && !vec_arg->next) {
                     fprintf(outfile, "printf(\"Max value of %s: %s\\n\", max_val_runtime(%s));\n", 
                            vec_arg->data.id.sval, "%f", vec_arg->data.id.sval);
                 } else {
                     fprintf(outfile, "/* Codegen Error: Invalid arguments for max_val */\n");
                 }
             } else if (strcmp(node->data.funcCall.name, "plot_xy") == 0) {
                 Node* x_arg = node->data.funcCall.args;
                 Node* y_arg = x_arg ? x_arg->next : NULL;
                 if (x_arg && x_arg->type == NODE_ID && 
                     y_arg && y_arg->type == NODE_ID && !y_arg->next) {
                     
                     fprintf(outfile, "if (write_xy_to_file(%s, %s, \"gnuplot_data.tmp\")) {\n", x_arg->data.id.sval, y_arg->data.id.sval);
                     fprintf(outfile, "%s    FILE *gnuplotPipe = open_gnuplot();\n", indentStr);
                     fprintf(outfile, "%s    if (gnuplotPipe) {\n", indentStr);
                     fprintf(outfile, "%s        fprintf(gnuplotPipe, \"plot 'gnuplot_data.tmp' using 1:2 with linespoints title '%s vs %s'\\n\");\n", indentStr, y_arg->data.id.sval, x_arg->data.id.sval );
                     fprintf(outfile, "%s        pclose(gnuplotPipe);\n%s}\n%s}\n", indentStr, indentStr, indentStr); 
                     fprintf(outfile, "%sremove(\"gnuplot_data.tmp\");\n", indentStr); // Clean up temp file

                 } else {
                      fprintf(outfile, "/* Codegen Error: Invalid arguments for plot_xy (expecting two vector IDs) */\n");
                 }
            } else if (strcmp(node->data.funcCall.name, "save_plot") == 0) {
                Node* file_arg = node->data.funcCall.args;
                 if (file_arg && file_arg->type == NODE_ID /* Allow string literals */ && !file_arg->next) {
                     // This needs to be called *before* the plot command
                     // How to coordinate? Requires restructuring or global state.
                     // Simple approach: Assume it sets global vars used by plot_xy
                     fprintf(outfile, "/* TODO: Set global flags for save_plot(\"%s\") */\n", file_arg->data.id.sval);
                 } else {
                      fprintf(outfile, "/* Codegen Error: Invalid argument for save_plot (expecting filename ID/string) */\n");
                 }
            } else if (strcmp(node->data.funcCall.name, "histogram") == 0) {
                 fprintf(outfile, "/* Histogram generation not fully implemented yet. */\n");
                 // TODO: Implement C-side binning, data file writing, gnuplot commands
            } else if (strcmp(node->data.funcCall.name, "load_vector") != 0) { // Avoid re-generating load_vector here 
                // Generic function call (if not handled elsewhere like assignment)
                fprintf(outfile, "%s(", node->data.funcCall.name);
                Node* arg = node->data.funcCall.args;
                int first_arg = 1;
                while (arg) {
                    if (!first_arg) fprintf(outfile, ", ");
                    generateExpressionCode(arg, outfile);
                    first_arg = 0;
                    arg = arg->next;
                }
                fprintf(outfile, ");\n"); 
            }
            break;
        default:
             // Print indentation for the error message too
            for (int i = 0; i < indentLevel; ++i) fprintf(stderr, "    "); 
            fprintf(stderr, "Codegen Error: Unsupported statement node type %d on line %d\n", node->type, node->lineno);
            break;
    }
}

// Helper to generate C code for expression nodes
static void generateExpressionCode(Node* node, FILE* outfile) {
    if (!node) return;

    switch (node->type) {
        case NODE_NUM:
            fprintf(outfile, "%f", node->data.dval);
            break;
        case NODE_ID:
            // Semantic Check: Ensure variable exists (basic check)
            if (!symtab_lookup(node->data.id.sval)) {
                 fprintf(stderr, "Codegen Error line %d: Use of undeclared identifier '%s'\n", node->lineno, node->data.id.sval);
                 fprintf(outfile, "/* Error: Undeclared ID %s */", node->data.id.sval); // Put error marker in C code
            } else {
                fprintf(outfile, "%s", node->data.id.sval);
            }
            break;
        case NODE_BINOP:
            // TODO: Add basic type checking here based on symbol table lookups of operands if they are IDs
            // e.g., if (getType(left) == TYPE_VECTOR || getType(right) == TYPE_VECTOR) { Error or Vector Op }
            fprintf(outfile, "(");
            generateExpressionCode(node->data.binOp.left, outfile);
            switch (node->data.binOp.op) {
                case OP_PLUS:  fprintf(outfile, " + "); break;
                case OP_MINUS: fprintf(outfile, " - "); break;
                case OP_STAR:  fprintf(outfile, " * "); break;
                case OP_DIV:   fprintf(outfile, " / "); break;
                default: fprintf(stderr, "Codegen Error: Unknown binary operator\n"); break;
            }
            generateExpressionCode(node->data.binOp.right, outfile);
            fprintf(outfile, ")");
            break;
         case NODE_UNARYOP:
             fprintf(outfile, "(");
             switch (node->data.unaryOp.op) {
                 case OP_UMINUS: fprintf(outfile, "-"); break;
                 default: fprintf(stderr, "Codegen Error: Unknown unary operator\n"); break;
             }
             generateExpressionCode(node->data.unaryOp.operand, outfile);
             fprintf(outfile, ")");
             break;
        case NODE_VEC: // Placeholder - how to generate C for a vector literal?
            fprintf(outfile, "/* Vector Literal Not Yet Implemented */"); 
            break;
        case NODE_FUNC_CALL: // Function call used within an expression
            // TODO: This assumes functions return values usable in expressions.
            // Need to define return types for built-ins.
            // For now, generate the call. This might not be valid C if void.
             if (strcmp(node->data.funcCall.name, "load_vector") == 0) {
                 fprintf(outfile, "/* load_vector used in expression - requires return value handling */");
             } else {
                 fprintf(outfile, "%s(", node->data.funcCall.name);
                 Node* arg = node->data.funcCall.args;
                 int first_arg = 1;
                 while (arg) {
                     if (!first_arg) fprintf(outfile, ", ");
                     generateExpressionCode(arg, outfile);
                     first_arg = 0;
                     arg = arg->next;
                 }
                 fprintf(outfile, ")");
             }
             break;
        // Add cases for function calls, etc. later
        
        default:
            fprintf(stderr, "Codegen Error: Unsupported expression node type %d on line %d\n", node->type, node->lineno);
            break;
    }
}

// Helper for print_vector
static void print_vector_runtime(Vector v, const char* name) {
    printf("Vector %s (size %zu): [", name, v.size);
    for (size_t i = 0; i < v.size; ++i) {
        printf("%f%s", v.data[i], (i == v.size - 1) ? "" : ", ");
    }
    printf("]\n");
}

// Helper for average
static double average_runtime(Vector v) {
    if (v.size == 0) return 0.0; // Or NaN?
    double sum = 0.0;
    for (size_t i = 0; i < v.size; ++i) {
        sum += v.data[i];
    }
    return sum / v.size;
}

// Helper for max_val
static double max_val_runtime(Vector v) {
    if (v.size == 0) return -INFINITY; // Or handle error
    double max = v.data[0];
    for (size_t i = 1; i < v.size; ++i) {
        if (v.data[i] > max) max = v.data[i];
    }
    return max;
}

// Helper to open gnuplot pipe
static FILE* open_gnuplot() {
    FILE* gp = popen("gnuplot -persist", "w");
    if (!gp) {
        fprintf(stderr, "Error opening gnuplot pipe.\n");
    }
    return gp;
}

// Helper to write vector data to temp file for gnuplot
// Returns 1 on success, 0 on failure
static int write_vector_to_file(Vector v, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return 0;
    for(size_t i = 0; i < v.size; ++i) {
        fprintf(f, "%f\n", v.data[i]);
    }
    fclose(f);
    return 1;
}
// Helper to write two vectors (X, Y) to temp file
static int write_xy_to_file(Vector x, Vector y, const char* filename) {
     if (x.size != y.size) {
        fprintf(stderr, "Error: X and Y vectors must have same size for plot_xy.\n");
        return 0;
     }
     FILE* f = fopen(filename, "w");
     if (!f) return 0;
     for(size_t i = 0; i < x.size; ++i) {
         fprintf(f, "%f %f\n", x.data[i], y.data[i]);
     }
     fclose(f); 
     return 1;
}

// Main code generation function
void generateCode(Node* astRoot, FILE* outfile) {
    if (!outfile) {
        fprintf(stderr, "Codegen Error: Output file is NULL\n");
        return;
    }

    // 1. Boilerplate Start
    fprintf(outfile, "#include <stdio.h>\n");
    fprintf(outfile, "#include <stdlib.h> // For malloc, free, exit, atof\n");
    fprintf(outfile, "#include <string.h> // For strtok_r, strcmp\n");
    fprintf(outfile, "#include <math.h> \n\n");

    // Define Vector struct in generated code
    fprintf(outfile, "// --- WizuAll Data Structures ---\n");
    fprintf(outfile, "typedef struct {\n");
    fprintf(outfile, "    double* data;\n");
    fprintf(outfile, "    size_t size;\n");
    fprintf(outfile, "} Vector;\n\n");

    // Define helper functions in generated code
    fprintf(outfile, "// --- WizuAll Runtime Helpers ---\n");
    // Count rows helper
    fprintf(outfile, "static size_t count_file_rows(const char* filename) {\n");
    fprintf(outfile, "    FILE* f = fopen(filename, \"r\");\n");
    fprintf(outfile, "    if (!f) { fprintf(stderr, \"Error opening file: %s\\n\", filename); return 0; }\n");
    fprintf(outfile, "    size_t count = 0; int ch;\n");
    fprintf(outfile, "    while (EOF != (ch = fgetc(f))) { if (ch == '\\n') count++; }\n");
    // TODO: Handle files not ending in newline? 
    fprintf(outfile, "    fclose(f); return count; \n}\n\n");
    // Read column helper
    fprintf(outfile, "static int read_double_column(const char* filename, int column, double* data, size_t max_rows) {\n");
    fprintf(outfile, "    FILE* f = fopen(filename, \"r\");\n");
    fprintf(outfile, "    if (!f) return 0;\n");
    fprintf(outfile, "    char line[2048]; size_t row = 0;\n"); // Increased buffer size
    fprintf(outfile, "    while (row < max_rows && fgets(line, sizeof(line), f)) {\n");
    fprintf(outfile, "        char* token; char* rest = line; int current_col = 0; int found = 0;\n");
    fprintf(outfile, "        while ((token = strtok_r(rest, \" \\t,\\n\", &rest))) {\n");
    fprintf(outfile, "            if (current_col == column) { data[row] = atof(token); found = 1; break; }\n");
    fprintf(outfile, "            current_col++;\n        }\n");
    // Add basic error if column not found? fprintf(outfile, " if (!found) { data[row]=0.0; /* Or handle error */ }\n");
    fprintf(outfile, "        row++;\n    }\n");
    fprintf(outfile, "    fclose(f); return 1;\n}\n\n");
    // Vector creation helper
    fprintf(outfile, "static Vector create_vector(size_t size) {\n");
    fprintf(outfile, "    Vector v; v.size = size; v.data = (double*)malloc(size * sizeof(double));\n");
    fprintf(outfile, "    if (!v.data && size > 0) { fprintf(stderr, \"Vector allocation failed\\n\"); exit(1); }\n");
    fprintf(outfile, "    for(size_t i=0; i<size; ++i) v.data[i] = 0.0; /* Initialize */ \n");
    fprintf(outfile, "    return v;\n}\n\n");
    // Vector free helper
     fprintf(outfile, "static void free_vector(Vector v) {\n");
     fprintf(outfile, "    free(v.data);\n}\n\n");

    fprintf(outfile, "// --- Main Program ---\n");
    fprintf(outfile, "int main() {\n");

    // 2. Variable Declarations
    fprintf(outfile, "    // Variable Declarations\n");
    if (globalSymTab) {
        Symbol* current = globalSymTab->head;
        while (current != NULL) {
            // Check type BEFORE generating declaration
            if (current->type == TYPE_VECTOR) {
                 fprintf(outfile, "    Vector %s; %s.data=NULL; %s.size=0; /* Initialized empty */\n", current->name, current->name, current->name);
            }
            else { // Treat UNDEFINED and SCALAR as double for now
                fprintf(outfile, "    double %s = 0.0; \n", current->name);
            }
            current = current->next;
        }
    }
    fprintf(outfile, "\n    // Code Body\n");

    // 3. Generate Code for Statements
    Node* currentStatement = astRoot;
    while (currentStatement != NULL) {
        generateStatementCode(currentStatement, outfile, 1); // Indent level 1 within main
        currentStatement = currentStatement->next;
    }

    // 4. Boilerplate End
    fprintf(outfile, "\n    return 0;\n");
    fprintf(outfile, "}\n");

    printf("C code generated successfully.\n");
} 