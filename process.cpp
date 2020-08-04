#include "process.hpp"
#include "vm.hpp"

Process :: Process() {
    program_size = PROCESS_DEFAULT_PROGRAM_SIZE;
    requested_memory = PROCESS_DEFAULT_REQUESTED_MEMORY;
};

void Process :: Work() {

};

void Process :: Wait() {

};

void Process :: Start() {
    
};

void Process :: SetProgramSize(uint64_t value) {
    program_size = value;
};

uint64_t Process :: GetProgramSize() {
    return program_size;
};

void Process :: SetRequestMemory(uint64_t value) {
    requested_memory = value;
};

uint64_t Process :: GetRequestedMemory() {
    return requested_memory;
};