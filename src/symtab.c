#include "symtab.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

// Definition of the global symbol table pointer
SymTab *globalSymTab = NULL;

void symtab_init() {
    if (globalSymTab != NULL) {
        // Already initialized? Maybe clean up first or just return
        fprintf(stderr, "Warning: Symbol table already initialized.\n");
        return; 
    }
    globalSymTab = (SymTab*)malloc(sizeof(SymTab));
    if (!globalSymTab) {
        fprintf(stderr, "Memory allocation error for symbol table\n");
        exit(EXIT_FAILURE);
    }
    globalSymTab->head = NULL;
    globalSymTab->count = 0;
    printf("Symbol table initialized.\n"); // Debug message
}

Symbol* symtab_lookup(const char *name) {
    if (!globalSymTab) return NULL; // Should not happen if init is called

    Symbol *current = globalSymTab->head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current; // Found
        }
        current = current->next;
    }
    return NULL; // Not found
}

Symbol* symtab_insert(const char *name, DataType type, int lineno) {
    if (!globalSymTab) {
         fprintf(stderr, "Error: Symbol table not initialized before insert.\n");
         exit(EXIT_FAILURE);
    }

    Symbol *existing = symtab_lookup(name);
    if (existing) {
        // Symbol exists, potentially update type if it was undefined
        // Or handle re-declaration errors if needed
        // For now, let's just update the type if it improves it
        if (existing->type == TYPE_UNDEFINED && type != TYPE_UNDEFINED) {
             printf("Updating type for symbol '%s'\n", name); // Debug
             existing->type = type;
        }
        // Maybe update line number? Or keep original? Depends on language semantics.
        // existing->declared_lineno = lineno; 
        return existing;
    }

    // Symbol does not exist, create and insert at head (simpler)
    Symbol *newSymbol = (Symbol*)malloc(sizeof(Symbol));
    if (!newSymbol) {
        fprintf(stderr, "Memory allocation error for new symbol '%s'\n", name);
        exit(EXIT_FAILURE);
    }
    newSymbol->name = strdup(name); // Own the name string
    if (!newSymbol->name) {
        fprintf(stderr, "Memory allocation error for symbol name '%s'\n", name);
        free(newSymbol);
        exit(EXIT_FAILURE);
    }
    newSymbol->type = type;
    newSymbol->declared_lineno = lineno;
    newSymbol->next = globalSymTab->head; // Link into list
    
    globalSymTab->head = newSymbol;
    globalSymTab->count++;

    printf("Inserted symbol '%s' (type %d) at line %d\n", name, type, lineno); // Debug
    return newSymbol;
}

void symtab_destroy() {
    if (!globalSymTab) return;

    Symbol *current = globalSymTab->head;
    Symbol *next;
    while (current != NULL) {
        next = current->next;
        free(current->name); // Free the duplicated name string
        free(current);       // Free the symbol struct itself
        current = next;
    }

    free(globalSymTab); // Free the table structure
    globalSymTab = NULL;
    printf("Symbol table destroyed.\n"); // Debug message
}

void symtab_print() {
     if (!globalSymTab) {
         printf("Symbol table not initialized.\n");
         return;
     }
     printf("--- Symbol Table ---\n");
     printf("Count: %d\n", globalSymTab->count);
     Symbol *current = globalSymTab->head;
     while (current != NULL) {
         const char* typeStr = "UNKNOWN";
         switch(current->type) {
             case TYPE_SCALAR: typeStr = "SCALAR"; break;
             case TYPE_VECTOR: typeStr = "VECTOR"; break;
             case TYPE_UNDEFINED: typeStr = "UNDEFINED"; break;
         }
         printf("  '%s' (Type: %s, Line: %d)\n", current->name, typeStr, current->declared_lineno);
         current = current->next;
     }
     printf("--------------------\n");
} 