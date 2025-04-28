%{
#include <stdio.h>
#include <string.h> // For strdup in lexer
#include "ast.h"     // AST definitions
#include "symtab.h"  // Symbol table definitions

// --- Global variable for the AST root (now potentially a statement list) ---
Node *astRoot = NULL;

// Forward declarations
int yylex(void); 
void yyerror(const char *s);

// External line number variable from lexer
extern int yylineno;
%}

// Define the type of values associated with tokens/rules
%union {
    double dval; // For lexer NUM value (before node creation)
    char*  sval; // For lexer ID value (before node creation)
    Node*  node; // AST Node pointer (used by most parser rules)
}

// Declare token types
%token <node> NUM      
%token <sval> ID       
%token T_EOF 0  
%token T_IF             // Keyword tokens
%token T_ELSE
%token T_WHILE

// Declare types for non-terminals
%type <node> program statement_list statement assignment_statement expression_statement 
%type <node> if_statement while_statement block
%type <node> expr term factor vector_literal expr_list optional_expr_list
%type <node> arg_list optional_arg_list // For function call arguments

// Define operator precedence and associativity
// Lowest precedence at the top
%right '='          // Assignment (right-associative)
%left '+' '-'     // Addition, Subtraction (left-associative)
%left '*' '/'     // Multiplication, Division (left-associative)
%right UMINUS       // Unary minus (pseudo-token for precedence)
%left T_ELSE // Lowest precedence for dangling else
// Parentheses, brackets, commas handled by grammar rules

%% // Rules section

// Start symbol: a program is a list of statements
program: statement_list 
            { astRoot = $1; /* Assign the final statement list to root */ }
       ;

statement_list: /* empty */ 
                  { $$ = NULL; }
              | statement_list statement 
                  { 
                    // Append statement to the list
                    if ($1 == NULL) { 
                        $$ = $2; // First statement
                    } else {
                        Node* current = $1;
                        while (current->next != NULL) {
                            current = current->next;
                        }
                        current->next = $2;
                        $$ = $1; // Return head of the list
                    }
                  }
              ;

statement: assignment_statement { $$ = $1; }
         | expression_statement { $$ = $1; }
         | if_statement         { $$ = $1; }
         | while_statement      { $$ = $1; }
         | block                { $$ = $1; /* Block itself can be a statement (contains list) */ }
         ;

assignment_statement: ID '=' expr ';' 
                      { 
                        Symbol* sym = symtab_insert($1, TYPE_UNDEFINED, @1.first_line); 
                        // Pass the original sval ($1) to newNodeAssign, which will strdup it.
                        $$ = newNodeAssign(@$.lineno, $1, $3);
                        // Do NOT free($1) here; symtab and AST node own copies.
                      }
                    ;

expression_statement: expr ';' 
                        { $$ = $1; /* Just pass the expression AST up */ }
                    | ';' 
                        { $$ = NULL; /* Allow empty statements */ }
                    ;

// If statement (handles optional else)
if_statement: T_IF '(' expr ')' block %prec T_ELSE // Higher precedence for if without else
                { $$ = newNodeIf(@1.first_line, $3, $5, NULL); }
            | T_IF '(' expr ')' block T_ELSE block 
                { $$ = newNodeIf(@1.first_line, $3, $5, $7); }
            ;

// While statement
while_statement: T_WHILE '(' expr ')' block
                   { $$ = newNodeWhile(@1.first_line, $3, $5); }
               ;

// Block of statements
block: '{' statement_list '}' 
         { $$ = $2; /* Return the head of the statement list within the block */ }
     ;

expr: expr '+' term   { $$ = newNodeBinaryOp(@$.lineno, OP_PLUS, $1, $3); }
    | expr '-' term   { $$ = newNodeBinaryOp(@$.lineno, OP_MINUS, $1, $3); }
    | term            { $$ = $1; }
    ;

term: term '*' factor { $$ = newNodeBinaryOp(@$.lineno, OP_STAR, $1, $3); }
    | term '/' factor { $$ = newNodeBinaryOp(@$.lineno, OP_DIV, $1, $3); }
    | factor          { $$ = $1; }
    ;

factor: '(' expr ')'           { $$ = $2; }
      | '-' factor %prec UMINUS { $$ = newNodeUnaryOp(@$.lineno, OP_UMINUS, $2); } 
      | NUM                      { $$ = newNodeNum(@$.lineno, $1); }
      | ID                       
          { 
            symtab_insert($1, TYPE_UNDEFINED, @$.lineno);
            $$ = newNodeID(@$.lineno, $1); 
          }
      | vector_literal           { $$ = $1; }
      | ID '(' optional_arg_list ')' // Function call
          { 
            // Need to handle potential re-use of $1 ID string
            $$ = newNodeFuncCall(@1.first_line, $1, $3); 
            free($1); // Free the ID string from lexer, newNodeFuncCall strdup'd it
          }
      ;

vector_literal: '[' optional_expr_list ']' 
                  { $$ = $2 ? $2 : newNodeVec(@$.lineno, NULL); /* Handle empty vector */ }
                ;

optional_expr_list: /* empty */ 
                      { $$ = NULL; }
                  | expr_list 
                      { $$ = $1; }
                  ;

// Builds a NODE_VEC containing a list of expression nodes
// Note: This approach creates a temporary list and then converts it. 
// A more direct approach might build the vec node incrementally.
expr_list: expr 
             { 
               // Create a new vector node with the first expression
               $$ = newNodeVec(@$.lineno, $1); 
             }
         | expr_list ',' expr 
             { 
               // Append the new expression node to the existing vector node
               $$ = appendToVec($1, $3); 
             }
         ;

// Argument list rules (similar to expr_list but use 'next' pointer directly)
optional_arg_list: /* empty */ 
                     { $$ = NULL; }
                 | arg_list 
                     { $$ = $1; }
                 ;

arg_list: expr 
            { 
              $$ = $1;       // First argument node
              $$->next = NULL; // Ensure it's the end of the list so far
            }
        | arg_list ',' expr 
            { 
              // Append $3 to the list headed by $1
              Node* current = $1;
              while (current->next != NULL) { // Find end of list
                  current = current->next;
              }
              current->next = $3; // Append new argument
              $3->next = NULL;    // Ensure it's the new end
              $$ = $1;            // Return the head of the list
            }
        ;

%% // C code section

// Define yyerror function
void yyerror(const char *s) {
    fprintf(stderr, "Syntax error near line %d: %s\n", yylineno, s);
}

// main function might go here or in main.c depending on structure
// int main() {
//     if (yyparse() == 0) {
//         printf("Parsing successful!\n");
//         return 0;
//     } else {
//         printf("Parsing failed.\n");
//         return 1;
//     }
// } 