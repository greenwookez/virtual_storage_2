#include <vector>
#include <ctime>
#include "sim.hpp"

using namespace std;

//TIME CONSTs
const SimulatorTime nanoSec = 1;
const SimulatorTime microSec = 1000000;
const SimulatorTime Sec = 1000000000;
const SimulatorTime Minute = Sec * 60;
const SimulatorTime Hour = Minute * 60;

//CONFIG
const SimulatorTime AE_DEFAULT_TIME_FOR_DATA_IO = 10*microSec;

const SimulatorTime CPU_DEFAULT_TIME_FOR_CONVERSION = 1;

const SimulatorTime OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT = 1000*nanoSec;
const uint64_t OS_DEFAULT_RAM_SIZE = 512;
const uint64_t OS_DEFAULT_DISKSPACE_SIZE = 5000;
const uint64_t OS_DEFAULT_TIME_FOR_ALLOCATION = 10*microSec;

const uint64_t PROCESS_DEFAULT_REQUESTED_MEMORY = 50;

//TYPES
typedef uint64_t PageNumber;
typedef PageNumber VirtualAddress;
typedef PageNumber RealAddress;
typedef PageNumber DiskAddress;


//HEADER FILE
struct RequestStruct;
class Process;

struct TTStruct {
    VirtualAddress vaddress;
    RealAddress raddress;
    bool is_valid;
};

class TT {
    Process* p_process;

    vector <TTStruct> records;

public:
    TT(Process* _p_process, PageNumber size);

    TTStruct& GetRecord(VirtualAddress vaddress);
    int GetSize();
    Process* GetProcess();
};

class RAM {
    vector <bool> ram; // true means already in use
public:
    RAM();
    bool GetRealAddress(PageNumber raddress);
    void SetRealAddress(PageNumber raddress, bool value);
    int GetSize();
};

class Requester {
    vector <RequestStruct> request_queue;
public:
    void AddRequest(Process* p_process, VirtualAddress vaddress, RealAddress raddress, bool load_flag);
    void DeleteRequest();
    RequestStruct GetRequest();
    bool IsEmpty();
};

class Scheduler {
    vector <Process*> process_queue;
public:
    void AddProcess(Process* p_process);
    void DeleteProcess(Process* p_process);
    void PutInTheEnd();
    Process* GetProcess();
    bool IsEmpty();
};

class OS : public Agent {
    vector <TT> translation_tables;
    RAM ram;
    Requester requester;
    Scheduler scheduler;

public:
    OS();
    void HandelInterruption(VirtualAddress vaddress, RealAddress raddress, Process* p_process);
    void LoadProcess(Process* _p_process);
    void Allocate(VirtualAddress vaddress, Process* p_process);
    void Substitute(VirtualAddress vaddress, Process* p_process);
    TT& FindTT(Process* p_process);
    Requester& GetRequester();
    Scheduler& GetScheduler();
    void ProcessQueue();
    void ChangeQueue();

    void Work();
    void Wait();
    void Start();
};

class CPU : public Agent {
public:
    CPU();
    void Convert(VirtualAddress vaddress, Process* p_process);

    void Work();
    void Wait();
    void Start();
};

class DiskSpace {
    vector <bool> disk;  // true means already in use

public:
    DiskSpace();
    bool GetDiskAddress(PageNumber daddress);
    void SetDiskAddress(PageNumber daddress, bool value);
    int GetSize();
};

class AE : public Agent {
    DiskSpace disk;

    struct SwapIndexStruct {
        Process* p_process;
        VirtualAddress vaddress;
        DiskAddress daddress;
    };

    vector <SwapIndexStruct> SwapIndex;

    void LoadData();
    void PopData();
public:
    AE();
    void ProcessRequest();
    bool IsLoaded(Process* p_process, VirtualAddress vaddress);

    void Work();
    void Wait();
    void Start();
};

class Process : public Agent {
    PageNumber requested_memory;
    SimulatorTime time_limit;
public:
    Process();
    void MemoryRequest(VirtualAddress vaddress);
    void SetRequestedMemory(uint64_t value);
    uint64_t GetRequestedMemory();
    void SetTimeLimit(SimulatorTime value);
    SimulatorTime GetTimeLimit();
    void Work();
    void Wait();
    void Start();


};

int randomizer(int max);

extern OS* g_pOS;
extern CPU* g_pCPU;
extern AE* g_pAE;

struct RequestStruct {
    bool load_flag;
    Process* p_process;
    VirtualAddress vaddress;
    RealAddress raddress;
};