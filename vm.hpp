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

const bool CONFIG_LOG_ENABLE_EMPTY_STRINGS = false; // включает пустые строки в логе
const SimulatorTime CONFIG_SIM_TIME_LIMIT = 4.35*Sec; // лимит времени работы симулятора

const SimulatorTime AE_DEFAULT_TIME_FOR_DATA_IO = 10*microSec; // время работы устройства ввода/ввывода
const uint64_t AE_DEFAULT_DISKSPACE_SIZE = 500; // размер файла подкачки в страницах

const SimulatorTime CPU_DEFAULT_TIME_FOR_CONVERSION = 1; // время на преобразование адреса процессом

const SimulatorTime OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT = 10; // время, на которое процессу дается ЦП (потом ЦП передается другуму претенденту в очереди)
const uint64_t OS_DEFAULT_RAM_SIZE = 512; // размер ОП в страницах
const uint64_t OS_DEFAULT_TIME_FOR_ALLOCATION = 10*microSec; // время на размещение

const SimulatorTime PROCESS_DEFAULT_WORK_TIME = 1; // время, за которое процесс совершает единицу работы
const uint64_t PROCESS_DEFAULT_REQUESTED_MEMORY = 50; // память, необходимая процессу в количестве страниц


//TYPES
typedef uint64_t PageNumber;
typedef PageNumber VirtualAddress;
typedef PageNumber RealAddress;
typedef PageNumber DiskAddress;


//HEADER FILE
class AgentVM : public Agent {
    public:
    void Log(string text); // Переопределение логирования для вывода загруженности системы в конце каждого сообщения
};

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
    void DeleteRequest(Process* p_process, VirtualAddress vaddress);
    RequestStruct GetRequest();
    bool IsEmpty();

    void PrintQueue();
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

class OS : public AgentVM {
    vector <TT> translation_tables;
    
    Requester requester;
    Scheduler scheduler;

public:
    RAM ram;
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

    //For logging

    float ComputeRML();

    void Work();
    void Wait();
    void Start();
};

class CPU : public AgentVM {
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

class AE : public AgentVM {
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
    SimulatorTime io_total_time;
    AE();
    void ProcessRequest();
    bool IsLoaded(Process* p_process, VirtualAddress vaddress);

    //For logging
    float ComputeAEL();
    double ComputePSL();
    

    void Work();
    void Wait();
    void Start();
};

class Process : public AgentVM {
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
