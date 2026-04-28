#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
    // 1.2 + 3.4 * -5.6
    // initialise the  virtual machine and an empty chunk of bytecode
    initVM();
    Chunk chunk;
    initChunk(&chunk);

    // 1.2
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    // 3.4
    constant = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    // 1.2 + 3.4
    writeChunk(&chunk, OP_ADD, 123);

    // 5.6
    constant = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    // (1.2 + 3.4) * 5.6
    writeChunk(&chunk, OP_MULTIPLY, 123);

    // negate: -(1.2 + 3.4) * 5.6
    writeChunk(&chunk, OP_NEGATE, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);

    freeVM();
    freeChunk(&chunk);
    return 0;
}