#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h" // Potentially needed if storing type info derived from AST

// Data type stored in the symbol table
typedef enum {
    TYPE_UNDEFINED, // Should not happen after parsing/analysis
    TYPE_SCALAR, 
    TYPE_VECTOR
    // Add other types like TYPE_FUNCTION later
} DataType;

// Structure for a single symbol
typedef struct Symbol {
    char *name;      // Symbol name (variable identifier)
    DataType type;   // Data type (scalar, vector, etc.)
    int declared_lineno; // Line number where declared/first assigned
    // Add more info later: scope level, value/pointer (for interpreter), etc.
    struct Symbol *next; // Pointer for linked list implementation
} Symbol;

// Symbol Table structure (simple linked list head)
typedef struct {
    Symbol *head;
    int count;
    // Add fields for hash table later if needed
} SymTab;

// --- Global Symbol Table (simplicity for now) ---
// Note: A better design passes the table around or uses context struct
extern SymTab *globalSymTab;

// --- Function Prototypes for symtab.c ---

/**
 * @brief Initializes the global symbol table.
 */
void symtab_init();

/**
 * @brief Looks up a symbol by name in the global table.
 * @param name The name of the symbol to find.
 * @return Pointer to the Symbol structure if found, NULL otherwise.
 */
Symbol* symtab_lookup(const char *name);

/**
 * @brief Inserts a new symbol or updates an existing one.
 * If the symbol exists, its type might be updated (e.g., from undefined).
 * If it doesn't exist, it's added to the table.
 * @param name The name of the symbol.
 * @param type The data type of the symbol.
 * @param lineno The line number for the declaration/assignment.
 * @return Pointer to the newly inserted or found Symbol structure.
 */
Symbol* symtab_insert(const char *name, DataType type, int lineno);

/**
 * @brief Frees all memory associated with the global symbol table.
 */
void symtab_destroy();

/**
 * @brief Prints the contents of the symbol table (for debugging).
 */
void symtab_print();


#endif // SYMTAB_H 