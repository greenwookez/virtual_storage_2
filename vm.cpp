#include "vm.hpp"

struct RequestStruct;


TT::TT(Process* _p_process, PageNumber size) {
    for (int i = 0; i < (int)size; i++) {
        records.push_back({ (VirtualAddress)i, 0, false });
    }
    p_process = _p_process;
}

TTStruct& TT::GetRecord(VirtualAddress vaddress) {
    for (int i = 0; i < (int)records.size(); i++) {
        if (records[i].vaddress == vaddress) {
            return records[i];
        }
    }

    __throw_logic_error("RECORD NOT FOUND");
}

int TT::GetSize() {
    return (int)records.size();
}

Process* TT::GetProcess() {
    return p_process;
}

RAM::RAM() {
    for (int i = 0; i < (int)OS_DEFAULT_RAM_SIZE; i++) {
        ram.push_back(false);
    }
}

bool RAM::GetRealAddress(PageNumber raddress) {
    return ram[raddress];
}

void RAM::SetRealAddress(PageNumber raddress, bool value) {
    ram[raddress] = value;
}

int RAM::GetSize() {
    return ram.size();
}

void Requester::AddRequest(Process* p_process, VirtualAddress vaddress, bool load_flag) {
    if (IsEmpty()) {
        Schedule(g_pSim->GetTime(), g_pAE, AE::ProcessRequest);
    }

    request_queue.push_back({ load_flag, p_process, vaddress });
}

void Requester::DeleteRequest(/*Process* p_process, VirtualAddress vaddress*/) {
    /*for (int i = 0; i < request_queue.size(); i++) {
        if (request_queue[i].p_process == p_process && request_queue[i].vaddress == vaddress) {
            
            request_queue.erase(request_queue.begin() + i);
            return;
        }
    }

    throw exception("REQUEST NOT FOUND");*/

    request_queue.erase(request_queue.begin());
}

RequestStruct Requester::GetRequest() {
    return request_queue[0];
}

bool Requester::IsEmpty() {
    return request_queue.empty();
}

void Scheduler::AddProcess(Process* p_process) {
    if (IsEmpty()) {
        // Если очередь пуста, планируем событие обработки этой очереди
        Schedule(g_pSim->GetTime(), g_pOS, OS::ProcessQueue);
    }

    // Добавляем процесс в очередь кандидатов
    process_queue.push_back(p_process);
}

void Scheduler::DeleteProcess(Process* p_process) {
    for (int i = 0; i < (int)process_queue.size(); i++) {
        if (process_queue[i] == p_process) {

            process_queue.erase(process_queue.begin() + i);
            return;
        }
    }

    // Если процесс не был найден в очереди кандидатов на ЦП, выбрасываем
    // исключение
    __throw_logic_error("PROCESS NOT FOUND IN QUEUE");
}

void Scheduler::PutInTheEnd() {
    // Ставим первый процесс в очереди в конец этой очереди
    // (Здесь речь идет об очереди кандидатов на ЦП)
    Process* tmp = process_queue[0];
    process_queue.erase(process_queue.begin());
    process_queue.push_back(tmp);
}

Process* Scheduler::GetProcess() {
    // Возвращаем указатель на первый в очереди процесс
    return process_queue[0];
}

bool Scheduler::IsEmpty() {
    return process_queue.empty();
}

void OS::ProcessQueue() {
    // Событие обработки очереди кандидатов на ЦП

    // Устанавливаем лимит по времени на работу одного процесса с ЦП
    scheduler.GetProcess()->SetTimeLimit(OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT);
    // Планируем саму работу процесса
    Schedule(GetTime(), scheduler.GetProcess(), Process::Work);
}

void OS::ChangeQueue() {
    scheduler.GetProcess()->SetTimeLimit(0);
    scheduler.PutInTheEnd();
    Schedule(GetTime(), g_pOS, OS::ProcessQueue);
}

void OS::HandelInterruption(VirtualAddress vaddress, Process* p_process) {
    Schedule(GetTime(), g_pOS, OS::Allocate, vaddress, p_process);

    // Если по виртуальному адресу vaddress есть данные в АС, необходимо их
    // выгрузить
    
    if (g_pAE->IsLoaded(p_process, vaddress)) {
        requester.AddRequest(p_process, vaddress, false);
    }
}

void OS::LoadProcess(Process* p_process) {
    // Создаём новую ТП для процесса
    translation_tables.push_back(TT(p_process, p_process->GetRequestedMemory()));

    // Добавляем этот процесс в очередь претендентов на ЦП
    scheduler.AddProcess(p_process);
}

void OS::Allocate(VirtualAddress vaddress, Process* p_process) {
    for (int i = 0; i < ram.GetSize(); i++) {
        if (ram.GetRealAddress(i) == false) {
            // Устанавливаем флаг распределенности для найденного
            ram.SetRealAddress(i, true);
            
            // Вносим изменение в ТП процесса о новом соответствии виртуального адреса
            // реальному
            FindTT(p_process).GetRecord(vaddress).raddress = i;
            // А также о том, что реальный адрес действителен
            FindTT(p_process).GetRecord(vaddress).is_valid = true;
            
            if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
                Schedule(GetTime(), g_pOS->GetScheduler().GetProcess(), Process::Work);
            } else {
                Schedule(GetTime(), g_pOS, OS::ChangeQueue);
            }

            return;
        }
    }
    
    // Если нераспределенного реального адреса найдено не было, необходимо перераспределение
    // существующих реальных адресов. Вызываем соответсвующую подпрограмму
    Substitute(vaddress, p_process);
}

void OS::Substitute(VirtualAddress vaddress, Process* p_process) {
    RealAddress candidate_raddress = (RealAddress)randomizer(ram.GetSize());
    Process* candidate_process;
    VirtualAddress candidate_vaddress;
    for (int i = 0; i < translation_tables.size(); i++) {
        for (int j = 0; j < translation_tables[i].GetSize(); i++) {
            if (translation_tables[i].GetRecord(j).raddress == candidate_raddress && translation_tables[i].GetRecord(j).is_valid == true) {
                // Из найденной записи выделяем виртуальный адрес и указатель на процесс,
                // у которого отнимаем память
                candidate_vaddress = translation_tables[i].GetRecord(j).vaddress;
                candidate_process = translation_tables[i].GetProcess();

                // Вносим изменение в запись в ТП у процесса, у которого отнимаем память
                translation_tables[i].GetRecord(j).is_valid = false;
                break;
            }
        }
    }
    
    // Если указатель на процесс или виртуальный адрес не изменились, значит в цикле
    // не была найдена информация о кандидата на перераспределение
    if (candidate_process == nullptr) {
        __throw_logic_error("NO INFO IN TTS FOUND FOR CANDIDATE");
    }

    // В очередь запросов добавляем новый запрос на загрузку данных в АС из виртуального
    // адреса кандидата
    requester.AddRequest(candidate_process, candidate_vaddress, true);

    if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
        Schedule(GetTime(), g_pOS->GetScheduler().GetProcess(), Process::Work);
    } else {
        Schedule(GetTime(), g_pOS, OS::ChangeQueue);
    }
}

TT& OS::FindTT(Process* p_process) {
    for (int i = 0; i < (int)translation_tables.size(); i++) {
        if (translation_tables[i].GetProcess() == p_process) {
            return translation_tables[i];
        }
    }

    __throw_logic_error("TT NOT FOUND");
}

Requester& OS::GetRequester() {
    return requester;
}

Scheduler& OS::GetScheduler() {
    return scheduler;
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
        Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS, OS::HandelInterruption, vaddress, p_process);
        return;
    }
    else {
        if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
            Schedule(GetTime(), g_pOS->GetScheduler().GetProcess(), Process::Work);
        } else {
            Schedule(GetTime(), g_pOS, OS::ChangeQueue);
        }
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



DiskSpace::DiskSpace() {
    for (int i = 0; i < (int)OS_DEFAULT_DISKSPACE_SIZE; i++) {
        disk.push_back(false);
    }
}

bool DiskSpace::GetDiskAddress(PageNumber daddress) {
    return disk[daddress];
}

void DiskSpace::SetDiskAddress(PageNumber daddress, bool value) {
    disk[daddress] = value;
}

int DiskSpace::GetSize() {
    return disk.size();
}

void AE::LoadData(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < disk.GetSize(); i++) {
        if (disk.GetDiskAddress(i) == false) {
            disk.SetDiskAddress(i, true);
            SwapIndex.push_back({ p_process, vaddress, (DiskAddress)i });
            g_pOS->GetRequester().DeleteRequest();
            Schedule(GetTime(), this, AE::ProcessRequest);
            return;
        }
    }

    __throw_logic_error("NO FREE DISK SPACE");
}

void AE::PopData(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < (int)SwapIndex.size(); i++) {
        if (SwapIndex[i].p_process == p_process && SwapIndex[i].vaddress == vaddress) {
            SwapIndex.erase(SwapIndex.begin() + i);
            disk.SetDiskAddress(i, false) ;
            
            g_pOS->GetRequester().DeleteRequest();
            Schedule(GetTime(), this, AE::ProcessRequest);
            return;
        }
    }

    __throw_logic_error("SWAP INDEX RECORD NOT FOUND");
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
}

bool AE::IsLoaded(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < SwapIndex.size(); i++) {
        if (SwapIndex[i].p_process == p_process && SwapIndex[i].vaddress == vaddress) {
            return true;
        }
    }

    return false;
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

void Process::Work() {
    

    // if (g_pSim->GetTime() >= time_limit) {
    //     g_pOS->GetScheduler().PutInTheEnd();
    //     Schedule(GetTime(), g_pOS, OS::ProcessQueue);
    // } else {
    //     Schedule();
    // }
}

void Process::Wait() {
    //
}

void Process::Start() {
    Schedule(GetTime(), g_pOS, OS::LoadProcess, this);
}

void Process::SetRequestedMemory(uint64_t value) {
    requested_memory = value;
}

uint64_t Process::GetRequestedMemory() {
    return requested_memory;
}

void Process::SetTimeLimit(SimulatorTime value) {
    time_limit = value;
}

SimulatorTime Process::GetTimeLimit() {
    return time_limit;
}

int randomizer(int max) {
    srand(unsigned(clock()));
    return rand() % max;
};
