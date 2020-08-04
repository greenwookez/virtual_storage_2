#include "sim.hpp"

class Process : public Agent {
    uint64_t program_size;
    uint64_t requested_memory;

    public:
    Process();
    void Work();
    void Wait();
    void Start();

    void SetProgramSize(uint64_t value);
    uint64_t GetProgramSize();

    void SetRequestMemory(uint64_t value);
    uint64_t GetRequestedMemory();
};