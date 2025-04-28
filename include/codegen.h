#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symtab.h"
#include <stdio.h> 

/**
 * @brief Generates C code from the Abstract Syntax Tree.
 * 
 * @param astRoot The root of the AST (likely the head of a statement list).
 * @param outfile The output file stream to write the C code to.
 */
void generateCode(Node* astRoot, FILE* outfile);


#endif // CODEGEN_H 