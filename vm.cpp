#include "vm.hpp"
class TT;
class Requester;
class Scheduler;
struct TTStruct;
struct RequestStruct;
struct SwapIndexStruct;

OS::TT::TT(Process* _p_process, PageNumber size) {
    for (int i = 0; i < size; i++) {
        records.push_back({ i, 0, false });
    }
    p_process = _p_process;
}

TTStruct& OS::TT::GetRecord(VirtualAddress vaddress) {
    for (int i = 0; i < records.size(); i++) {
        if (records[i].vaddress == vaddress) {
            return records[i];
        }
    }

    throw exception("RECORD NOT FOUND");
}

TTStruct& OS::TT::GetRecord(RealAddress raddress) {
    for (int i = 0; i < records.size(); i++) {
        if (records[i].raddress == raddress) {
            return records[i];
        }
    }

    throw exception("RECORD NOT FOUND");
}

Process* OS::TT::GetProcess() {
    return p_process;
}

OS::RAM::RAM() {
    for (int i = 0; i < DEFAULT_RAM_SIZE; i++) {
        ram.push_back(false);
    }
}

bool& OS::RAM::GetRealAddress(PageNumber raddress) {
    return ram[raddress];
}

int OS::RAM::GetSize() {
    return ram.size();
}

void OS::Requester::AddRequest(Process* p_process, VirtualAddress vaddress, bool load_flag) {
    if (IsEmpty()) {
        /* ���� �� ������ ���������� ������� � �������, ��������� �����, ��
        ���� ��������� �������, �������������� �������. */
        Schedule(GetTime(), g_pAE, AE::ProcessRequest);
    }

    request_queue.push_back({ load_flag, p_process, vaddress });
}

void OS::Requester::DeleteRequest(/*Process* p_process, VirtualAddress vaddress*/) {
    /*for (int i = 0; i < request_queue.size(); i++) {
        if (request_queue[i].p_process == p_process && request_queue[i].vaddress == vaddress) {
            
            request_queue.erase(request_queue.begin() + i);
            return;
        }
    }

    throw exception("REQUEST NOT FOUND");*/

    request_queue.erase(request_queue.begin());
}

RequestStruct OS::Requester::GetRequest() {
    return request_queue[0];
}

bool OS::Requester::IsEmpty() {
    return request_queue.empty();
}

void OS::Scheduler::AddProcess(Process* p_process) {
    if (IsEmpty()) {
        Schedule(GetTime(), this, OS::ProcessQueue);
    }

    process_queue.push_back(p_process);
}

void OS::Scheduler::DeleteProcess(Process* p_process) {
    for (int i = 0; i < process_queue.size(); i++) {
        if (process_queue[i] == p_process) {

            process_queue.erase(process_queue.begin() + i);
            return;
        }
    }

    throw exception("PROCESS NOT FOUND IN QUEUE");
}

void OS::Scheduler::PutInTheEnd() {
    Process* tmp = process_queue[0];
    process_queue.erase(process_queue.begin());
    process_queue.push_back(tmp);
}

Process* OS::Scheduler::GetProcess() {
    return process_queue[0];
}

bool OS::Scheduler::IsEmpty() {
    return process_queue.empty();
}

void OS::HandelInterruption(VirtualAddress vaddress, Process* p_process) {

}

void OS::LoadProcess(Process* p_process) {
    TT tmp(p_process, p_process->GetRequestedMemory());
    translation_tables.push_back(tmp);

    scheduler.AddProcess(p_process);
}

void OS::Allocate(VirtualAddress vaddress, Process* p_process) {
    for (int i = 0; i < ram.GetSize(); i++) {
        if (ram.GetRealAddress(i) == false) {
            ram.GetRealAddress(i) = true;
            
            FindTT(p_process).GetRecord(vaddress).raddress = i;
            FindTT(p_process).GetRecord(vaddress).is_valid = true;
            return;
        }
    }

    Substitute(vaddress, p_process);
}

void OS::Substitute(VirtualAddress vaddress, Process* p_process) {
    /* ����� ��������� ��� �������������. � ������ ������ - ��������. */
    RealAddress candidate = (RealAddress)randomizer(ram.GetSize());
    
    requester.AddRequest(p_process, vaddress, true);
}

TT& OS::FindTT(Process* p_process) {
    for (int i = 0; i < translation_tables.size(); i++) {
        if (translation_tables[i].GetProcess() == p_process) {
            return translation_tables[i];
        }
    }

    throw exception("TT NOT FOUND");
}

Requester& OS::GetRequester() {
    return requester;
}

Scheduler& OS::GetScheduler() {
    return scheduler;
}


void OS::ProcessQueue() {
    SimulatorTime time_limit = g_pSim->Get_time() + OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT;
    Schedule(GetTime(), scheduler.GetProcess(), Process::Work, time_limit);
}

void OS::Work() {
    //
}

void OS::Wait() {
    //
}

void OS::Start() {
    //
}



void CPU::Convert(VirtualAddress vaddress, Process *p_process) {
    if (g_pOS->FindTT(p_process).GetRecord(vaddress).is_valid == false) {
        Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_COMVERSION, g_pOS, OS::HandelInterruption, vaddress, p_process);
        return;
    }
    else {
        // Sucessful conversion
    }
}

void CPU::Work() {
    //
}

void CPU::Wait() {
    //
}

void CPU::Start() {
    //
}



AE::DiskSpace::DiskSpace() {
    for (int i = 0; i < OS_DEFAULT_DISKSPACE_SIZE; i++) {
        disk.push_back(false);
    }
}

bool& AE::DiskSpace::GetDiskAddress(PageNumber daddress) {
    return disk[daddress];
}

int AE::DiskSpace::GetSize() {
    return disk.size();
}

void AE::LoadData(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < disk.GetSize(); i++) {
        if (disk.GetDiskAddress(i) == false) {
            disk.GetDiskAddress(i) = true;
            SwapIndex.push_back({ p_process, vaddress, i });
            g_pOS->GetRequester().DeleteRequest();
            Schedule(GetTime(), this, AE::ProcessRequest);
            return;
        }
    }

    throw exception("NO FREE DISK SPACE");
}

void AE::PopData(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < SwapIndex.size(); i++) {
        if (SwapIndex[i].p_process == p_process && SwapIndex[i].vaddress == vaddress) {
            SwapIndex.erase(SwapIndex.begin() + i);
            disk.GetDiskAddress(i) = false;
            
            g_pOS->GetRequester().DeleteRequest();
            Schedule(GetTime(), this, AE::ProcessRequest);
            return;
        }
    }

    throw exception("SWAP INDEX RECORD NOT FOUND");
}

void AE::ProcessRequest() {
    RequestStruct tmp = g_pOS->GetRequester().GetRequest();
    if (tmp.load_flag) {
        Schedule(GetTime(), this, AE::LoadData, tmp.p_process, tmp.loading_address);
    }
    else {
        Schedule(GetTime(), this, AE::PopData, tmp.p_process, tmp.loading_address);
    }
    return;
};

void AE::Work() {
    //
}

void AE::Wait() {
    //
}

void AE::Start() {
    //
}



Process::Process() {
    requested_memory = PROCESS_DEFAULT_REQUESTED_MEMORY;
};

void Process::Work(SimulatorTime time_limit) {
    /*���, ����������� ������*/

    if (g_pSim->GetTime() >= time_limit) {
        g_pOS->GetScheduler().PutInTheEnd();
        Schedule(GetTime(), g_pOS, OS::ProcessQueue);
    }
}

void Process::Wait() {
    //
}

void Process::Start() {
    //
}

void Process::SetRequestedMemory(uint64_t value) {
    requested_memory = value;
}

uint64_t Process::GetRequestedMemory() {
    return requested_memory;
}



int randomizer(int max) { // �������, ������������ ����� ����� �� 0 �� max-1.
    srand(unsigned(clock()));
    return rand() % max;
};
