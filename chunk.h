// chunk.h defines our code representation/ sequences of bytecode instructions

#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

// defines a Chunk that holds a pointer to an array of bytes(uint8_t*) - which will be our bytcode instructions
// Each byte is one instruction(or operand)
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines; // parallel array to code that holds the source line for each instruction
    ValueArray constants;
} Chunk;


void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
// appends a byte to the end of the chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
#endif