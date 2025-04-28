%{
#include "parser.tab.h"
#include <stdlib.h>
#include <string.h>
#include "ast.h"
int yylineno = 1;
%}

/* Optional: Define reusable patterns */
DIGIT    [0-9]
ID_START [a-zA-Z_]
ID_CONT  [a-zA-Z0-9_]

%%
// Skip whitespace and count newlines
[ \t\n]+   { 
              for (int i = 0; yytext[i] != '\0'; i++) {
                  if (yytext[i] == '\n') {
                      yylineno++;
                  }
              }
            } /* skip */ ;

// Comments (example: C-style single line)
"//".*      { /* Ignore comments */ }

// Number literal (integer or float)
{DIGIT}+(\.{DIGIT}*)?|\.{DIGIT}+  {
                              yylval.dval = atof(yytext); // Store double value in union
                              return NUM;                 // Return NUM token type
                            }

// Keywords
"if"                  { return T_IF; }
"else"                { return T_ELSE; }
"while"               { return T_WHILE; }

// Identifiers (must come after keywords to avoid matching keywords as IDs)
{ID_START}{ID_CONT}*  {
                        yylval.sval = strdup(yytext);
                        if (!yylval.sval) { /* Handle memory error */ exit(EXIT_FAILURE); }
                        return ID;
                      }

// Operators, Delimiters, etc.
// Return character code for simple tokens, let parser handle them
[-+*/()=;,\[\]]      { return yytext[0]; }

// End of file
<<EOF>>               { return T_EOF; } // Use the T_EOF token defined in parser

// Unrecognized character
.                     {
                        fprintf(stderr, "Error line %d: Unrecognized character '%s'\n", yylineno, yytext);
                      }

%%
int yywrap() { return 1; }

// Optional: If main is not in parser.y
// int main(int argc, char **argv) {
//     // Initialization if needed
//     yylex();
//     return 0;
// } 