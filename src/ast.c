#include "ast.h"
#include <stdio.h>  // For printf, fprintf
#include <stdlib.h> // For malloc, exit, realloc
#include <string.h> // For strdup, free

// Helper for printing indentation
static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// --- Node Constructors ---

// Generic node allocation helper (internal)
static Node* createNode(int lineno, NodeType type) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Memory allocation error line %d\n", lineno);
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->next = NULL;
    node->lineno = lineno;
    // Initialize data union members to NULL/0 where applicable
    memset(&node->data, 0, sizeof(node->data)); 
    return node;
}

Node* newNodeNum(int lineno, double val) {
    Node* node = createNode(lineno, NODE_NUM);
    node->data.dval = val;
    return node;
}

Node* newNodeBinaryOp(int lineno, OpType op, Node* left, Node* right) {
    Node* node = createNode(lineno, NODE_BINOP);
    node->data.binOp.op = op;
    node->data.binOp.left = left;
    node->data.binOp.right = right;
    return node;
}

Node* newNodeUnaryOp(int lineno, OpType op, Node* operand) {
    Node* node = createNode(lineno, NODE_UNARYOP);
    node->data.unaryOp.op = op;
    node->data.unaryOp.operand = operand;
    return node;
}

Node* newNodeID(int lineno, char* sval) {
    Node* node = createNode(lineno, NODE_ID);
    node->data.id.sval = strdup(sval); // Duplicate string to own it
    if (!node->data.id.sval) {
         fprintf(stderr, "Memory allocation error for ID string line %d\n", lineno);
         exit(EXIT_FAILURE);
    }
    return node;
}

Node* newNodeVec(int lineno, Node* firstElement) {
    Node* node = createNode(lineno, NODE_VEC);
    node->data.vec.count = 0;
    node->data.vec.elements = NULL;
    if (firstElement) {
        return appendToVec(node, firstElement);
    }
    return node;
}

// Appends an element node to a vector node's list
Node* appendToVec(Node* vecNode, Node* element) {
    if (!vecNode || vecNode->type != NODE_VEC || !element) {
        fprintf(stderr, "Internal error: Invalid appendToVec call\n");
        // Maybe return vecNode or NULL depending on desired error handling
        return vecNode; 
    }
    vecNode->data.vec.count++;
    vecNode->data.vec.elements = (Node**)realloc(vecNode->data.vec.elements, vecNode->data.vec.count * sizeof(Node*));
    if (!vecNode->data.vec.elements) {
        fprintf(stderr, "Memory allocation error for vector elements line %d\n", vecNode->lineno);
        exit(EXIT_FAILURE);
    }
    vecNode->data.vec.elements[vecNode->data.vec.count - 1] = element;
    return vecNode;
}

Node* newNodeAssign(int lineno, char* name, Node* value) {
    Node* node = createNode(lineno, NODE_ASSIGN);
    node->data.assignOp.name = strdup(name); 
    if (!node->data.assignOp.name) {
         fprintf(stderr, "Memory allocation error for assignment name line %d\n", lineno);
         exit(EXIT_FAILURE);
    }
    node->data.assignOp.value = value;
    return node;
}

Node* newNodeIf(int lineno, Node* condition, Node* then_branch, Node* else_branch) {
    Node* node = createNode(lineno, NODE_IF);
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.then_branch = then_branch;
    node->data.ifStmt.else_branch = else_branch;
    return node;
}

Node* newNodeWhile(int lineno, Node* condition, Node* body) {
    Node* node = createNode(lineno, NODE_WHILE);
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.body = body;
    return node;
}

Node* newNodeFuncCall(int lineno, char* name, Node* args) {
    Node* node = createNode(lineno, NODE_FUNC_CALL);
    node->data.funcCall.name = strdup(name); // Own the name
     if (!node->data.funcCall.name) {
         fprintf(stderr, "Memory allocation error for function call name line %d\n", lineno);
         exit(EXIT_FAILURE);
    }
    node->data.funcCall.args = args; // Argument list head
    return node;
}

// --- AST Traversal/Utility Functions ---

void printAST(Node* node, int indent) {
    if (!node) return;
    printIndent(indent);
    printf("Line %d: ", node->lineno);

    switch (node->type) {
        case NODE_NUM:
            printf("Number: %f\n", node->data.dval);
            break;
        case NODE_BINOP:
            printf("Binary Op: ");
            switch (node->data.binOp.op) {
                case OP_PLUS: printf("+\n"); break;
                case OP_STAR: printf("*\n"); break;
                case OP_MINUS: printf("-\n"); break;
                case OP_DIV: printf("/\n"); break;
                default: printf("Unknown\n");
            }
            printAST(node->data.binOp.left, indent + 1);
            printAST(node->data.binOp.right, indent + 1);
            break;
        case NODE_UNARYOP:
             printf("Unary Op: ");
            switch (node->data.unaryOp.op) {
                case OP_UMINUS: printf("-\n"); break;
                default: printf("Unknown\n");
            }
            printAST(node->data.unaryOp.operand, indent + 1);
            break;
        case NODE_ID:
            printf("Identifier: %s\n", node->data.id.sval);
            break;
        case NODE_VEC:
            printf("Vector (count=%zu):\n", node->data.vec.count);
            for (size_t i = 0; i < node->data.vec.count; i++) {
                printAST(node->data.vec.elements[i], indent + 1);
            }
            break;
        case NODE_ASSIGN:
            printf("Assignment: %s =\n", node->data.assignOp.name);
            printAST(node->data.assignOp.value, indent + 1);
            break;
        case NODE_IF:
            printf("If Statement:\n");
            printIndent(indent + 1); printf("Condition:\n");
            printAST(node->data.ifStmt.condition, indent + 2);
            printIndent(indent + 1); printf("Then Branch:\n");
            // Print the list of statements in the then branch
            Node* thenStmt = node->data.ifStmt.then_branch;
            while(thenStmt) { printAST(thenStmt, indent + 2); thenStmt = thenStmt->next; }
            if (node->data.ifStmt.else_branch) {
                printIndent(indent + 1); printf("Else Branch:\n");
                // Print the list of statements in the else branch
                Node* elseStmt = node->data.ifStmt.else_branch;
                while(elseStmt) { printAST(elseStmt, indent + 2); elseStmt = elseStmt->next; }
            }
            break;
        case NODE_WHILE:
            printf("While Statement:\n");
            printIndent(indent + 1); printf("Condition:\n");
            printAST(node->data.whileStmt.condition, indent + 2);
            printIndent(indent + 1); printf("Body:\n");
            // Print the list of statements in the body
            Node* bodyStmt = node->data.whileStmt.body;
            while(bodyStmt) { printAST(bodyStmt, indent + 2); bodyStmt = bodyStmt->next; }
            break;
        case NODE_FUNC_CALL:
            printf("Function Call: %s\n", node->data.funcCall.name);
            if (node->data.funcCall.args) {
                printIndent(indent + 1); printf("Arguments:\n");
                Node* arg = node->data.funcCall.args;
                while(arg) {
                    printAST(arg, indent + 2);
                    arg = arg->next;
                }
            }
            break;
        default:
            printf("Unknown Node Type\n");
    }
    // NOTE: The list printing for statement lists is now handled *within* the IF/WHILE/Program printing
    // We don't call printAST(node->next, indent) at the end here anymore
}

void freeAST(Node* node) {
    if (!node) return;
    
    // Recursively free children based on type
    switch (node->type) {
        case NODE_NUM:
            break; // No children, no allocated data
        case NODE_BINOP:
            freeAST(node->data.binOp.left);
            freeAST(node->data.binOp.right);
            break;
        case NODE_UNARYOP:
            freeAST(node->data.unaryOp.operand);
            break;
        case NODE_ID:
            free(node->data.id.sval); // Free the duplicated string
            break;
        case NODE_VEC:
            // Free each element's AST, then the elements array itself
            for (size_t i = 0; i < node->data.vec.count; i++) {
                freeAST(node->data.vec.elements[i]);
            }
            free(node->data.vec.elements);
            break;
        case NODE_ASSIGN:
            free(node->data.assignOp.name); 
            freeAST(node->data.assignOp.value);
            break;
        case NODE_IF:
            freeAST(node->data.ifStmt.condition);
            freeAST(node->data.ifStmt.then_branch); // Free the list starting from the head
            if (node->data.ifStmt.else_branch) {
                freeAST(node->data.ifStmt.else_branch); // Free the list starting from the head
            }
            break;
        case NODE_WHILE:
            freeAST(node->data.whileStmt.condition);
            freeAST(node->data.whileStmt.body); // Free the list starting from the head
            break;
        case NODE_FUNC_CALL:
            free(node->data.funcCall.name);
            freeAST(node->data.funcCall.args); // Free argument list
            break;
        default:
             fprintf(stderr, "Warning: Trying to free unknown node type %d\n", node->type);
             break;
    }
    
    // Free the next node in the sequence *after* processing the current node's children
    freeAST(node->next);

    // Finally, free the node structure itself
    free(node);
} 