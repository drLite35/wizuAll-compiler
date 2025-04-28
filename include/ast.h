#ifndef AST_H
#define AST_H

#include <stdlib.h> // For size_t
#include <string.h> // For strdup (might be needed in parser/lexer)

// Forward declaration
typedef struct Node Node;

// Type of node in the AST
typedef enum {
    NODE_NUM,      // Simple number
    NODE_BINOP,    // Binary operation (+, *, -, /)
    NODE_UNARYOP,  // Unary operation (e.g., -)
    NODE_ID,       // Identifier
    NODE_VEC,      // Vector literal
    NODE_ASSIGN,   // Assignment statement
    NODE_IF,       // If statement
    NODE_WHILE,    // While statement
    NODE_FUNC_CALL // Function call
    // Add other types later (e.g., NODE_FUNC_CALL, NODE_IF)
} NodeType;

// Type of binary/unary operator
typedef enum {
    // Binary
    OP_PLUS,
    OP_STAR,
    OP_MINUS,
    OP_DIV,
    // Unary
    OP_UMINUS // Unary minus
    // Add other operators later
} OpType;

// The AST node structure
struct Node {
    NodeType type;
    Node *next; // For lists of statements/expressions
    int lineno; // Store line number for errors/debugging

    union {
        // NODE_NUM
        double dval;
        // NODE_BINOP
        struct {
            OpType op;
            Node *left;
            Node *right;
        } binOp;
        // NODE_UNARYOP
        struct {
            OpType op;
            Node *operand;
        } unaryOp;
        // NODE_ID
        struct {
            char *sval; // Name of the identifier
        } id;
        // NODE_VEC
        struct {
            Node **elements; // Dynamically allocated array of element nodes
            size_t count;    // Number of elements
        } vec;
        // NODE_ASSIGN
        struct {
            char *name;    // Name of the variable being assigned to
            Node *value;   // AST subtree for the value being assigned
        } assignOp;
        // NODE_IF
        struct {
            Node *condition;
            Node *then_branch; // Head of statement list for if-block
            Node *else_branch; // Head of statement list for else-block (can be NULL)
        } ifStmt;
        // NODE_WHILE
        struct {
            Node *condition;
            Node *body;      
        } whileStmt;
        // NODE_FUNC_CALL
        struct {
            char *name;      // Function name
            Node *args;      // Head of argument list (linked via 'next')
        } funcCall;
    } data;
};

// --- Function Prototypes for ast.c ---

Node* newNodeNum(int lineno, double val);
Node* newNodeBinaryOp(int lineno, OpType op, Node* left, Node* right);
Node* newNodeUnaryOp(int lineno, OpType op, Node* operand);
Node* newNodeID(int lineno, char* sval);
Node* newNodeVec(int lineno, Node* firstElement); // Initially create with one element
Node* appendToVec(Node* vecNode, Node* element); // Helper to add elements during parsing
Node* newNodeAssign(int lineno, char* name, Node* value);
Node* newNodeIf(int lineno, Node* condition, Node* then_branch, Node* else_branch);
Node* newNodeWhile(int lineno, Node* condition, Node* body);
Node* newNodeFuncCall(int lineno, char* name, Node* args);

void printAST(Node* node, int indent);
void freeAST(Node* node);


#endif // AST_H 