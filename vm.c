// defines functions to create and tear down a virtual machine

#include "common.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>


VM vm;

static void resetStack(){
    vm.stackTop = vm.stack;
}

void initVM(){
    resetStack();
}

void freeVM(){}


//push a new value ontop the top of the stack
void push(Value value){
    *vm.stackTop = value; // dereference the stack top pointer to store the value at the current top of the stack
    vm.stackTop++; // increment stackTop to point to the next empty slot
}

// pop the most recently pushed value back off and return it
Value pop(){
    vm.stackTop--; // decrement stackTop to point back to the most recently pushed value
    return *vm.stackTop; // dereference stackTop to return the value at the top of the stack
}

static InterpretResult run(){
    #define READ_BYTE() (*vm.ip++) // reads the byte currently pointed at by ip and then advances the instruction pointer
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()]) // reads the next byte from the bytecode, treats the resulting number as an index and looks up the corresponding Value in the chunk's constant table
    #define BINARY_OP(op) \
        do { \
            double b = pop(); \
            double a = pop(); \
            push(a op b); \
        } while(false)
    
        for(;;){

#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++){
            printf("[ ");
            printValue(*slot); // print the value at the current slot
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        // fetch, decode, and execute the instruction
        switch (instruction = READ_BYTE()){
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NEGATE: push(-pop()); break;

            case OP_RETURN:{
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
    #undef READ_BYTE // clean up the acro after the function so t doesn't leak into the rest of the file
    #undef READ_CONSTANT
    #undef BINARY_OP
}

InterpretResult interpret(Chunk* chunk){
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run(); // an nternal helper function that runs the bytecode instructions
}