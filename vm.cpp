#include "vm.hpp"
#include <iostream>
#include <iomanip>
struct RequestStruct;

TT::TT(Process* _p_process, PageNumber size) {
    for (int i = 0; i < static_cast<int>(size); i++) {
        records.push_back({ static_cast<VirtualAddress>(i), 0, false });
    }
    p_process = _p_process;
}

TTStruct& TT::GetRecord(VirtualAddress vaddress) {
    for (int i = 0; i < static_cast<int>(records.size()); i++) {
        if (records[i].vaddress == vaddress) {
            return records[i];
        }
    }

    __throw_logic_error("RECORD NOT FOUND");
}

int TT::GetSize() {
    return static_cast<int>(records.size());
}

Process* TT::GetProcess() {
    return p_process;
}

RAM::RAM() {
    for (int i = 0; i < static_cast<int>(OS_DEFAULT_RAM_SIZE); i++) {
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

void Requester::AddRequest(Process* p_process, VirtualAddress vaddress, RealAddress raddress, bool load_flag) {
    if (IsEmpty()) {
        Schedule(g_pSim->GetTime(), g_pAE, AE::ProcessRequest);
    }

    string text = "New request (" + p_process->GetName() + " VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " RA=" + string(4 - to_string(raddress).length(), '0') + to_string(raddress) + " LF=" + string(4 - to_string(load_flag).length(), '0') + to_string(load_flag) + ")";
    PrintTime(&std::cout);
    std::cout << " " << std::setw(10) << std::setfill(' ') << std::right << "Requester"
        << "   " << text << std::endl;
    request_queue.push_back({ load_flag, p_process, vaddress, raddress });
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


    string text = "Delete request (" + request_queue[0].p_process->GetName() + " VA=" + string(4 - to_string(request_queue[0].vaddress).length(), '0') + to_string(request_queue[0].vaddress) + " RA=" + string(4 - to_string(request_queue[0].raddress).length(), '0') + to_string(request_queue[0].raddress) + " LF=" + string(4 - to_string(request_queue[0].load_flag).length(), '0') + to_string(request_queue[0].load_flag) + ")";
    PrintTime(&std::cout);
    std::cout << " " << std::setw(10) << std::setfill(' ') << std::right << "Requester"
        << "   " << text << std::endl;
}

RequestStruct Requester::GetRequest() {
    return request_queue[0];
}

bool Requester::IsEmpty() {
    return request_queue.empty();
}

void Requester::PrintQueue() {
    if (request_queue.size() == 0) {
        std::cout << "Request Queue is Empty." << endl;
    } else {
        for (int i = 0; i < request_queue.size(); i++) {
            std::cout << "{" << request_queue[i].load_flag << "} {" <<
            request_queue[i].p_process << "} {" << request_queue[i].vaddress <<
            "} {" << request_queue[i].raddress << "}" << endl;
        }
    }
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
    for (int i = 0; i < static_cast<int>(process_queue.size()); i++) {
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
    scheduler.GetProcess()->SetTimeLimit(GetTime() + OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT);
    // Планируем саму работу процесса
    Schedule(GetTime(), scheduler.GetProcess(), Process::Work);
}

void OS::ChangeQueue() {
    scheduler.GetProcess()->SetTimeLimit(0);
    scheduler.PutInTheEnd();
    Schedule(GetTime(), g_pOS, OS::ProcessQueue);
}

OS::OS() {
    SetName("OS");
}

void OS::HandelInterruption(VirtualAddress vaddress, RealAddress raddress,  Process* p_process) {
    // Ранее здесь Allocate планирвоался, а не вызывался
    Allocate(vaddress, p_process);

    // Если по виртуальному адресу vaddress есть данные в АС, необходимо их
    // выгрузить
    
    if (g_pAE->IsLoaded(p_process, vaddress)) {
        requester.AddRequest(p_process, vaddress, raddress, false);
    }
}

void OS::LoadProcess(Process* p_process) {
    // Создаём новую ТП для процесса
    translation_tables.push_back(TT(p_process, p_process->GetRequestedMemory()));

    // Добавляем этот процесс в очередь претендентов на ЦП
    scheduler.AddProcess(p_process);
}

void OS::Allocate(VirtualAddress vaddress, Process* p_process) {
    for (int i = 0; i < static_cast<int>(ram.GetSize()); i++) {
        if (ram.GetRealAddress(i) == false) {
            // Устанавливаем флаг распределенности для найденного
            ram.SetRealAddress(i, true);
            
            // Вносим изменение в ТП процесса о новом соответствии виртуального адреса
            // реальному
            FindTT(p_process).GetRecord(vaddress).raddress = i;
            // А также о том, что реальный адрес действителен
            FindTT(p_process).GetRecord(vaddress).is_valid = true;

            if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
                Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS->GetScheduler().GetProcess(), Process::Work);
            } else {
                Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS, OS::ChangeQueue);
            }


            Log("Allocate RA=" + string(4 - to_string(i).length(), '0') + to_string(i) + " as VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + ", resume");
            return;
        }
    }
    
    // Если нераспределенного реального адреса найдено не было, необходимо перераспределение
    // существующих реальных адресов. Вызываем соответсвующую подпрограмму
    Substitute(vaddress, p_process);
}

void OS::Substitute(VirtualAddress vaddress, Process* p_process) {
    /*
        Стоит необходимость изменить алгоритм перераспределения.

        Попробовать перебирать все адреса в прямом порядке, пока не встретится подходящий.
    */
    RealAddress candidate_raddress;
    Process* candidate_process;
    VirtualAddress candidate_vaddress;

    bool found_flag = false;
    for (int i = 0; i < ram.GetSize(); i++) { // цикл по адресам реальной памяти
        for (int j =0; j < static_cast<int>(translation_tables.size()); j++) { // цикл по всем ТП
            for (int k = 0; k < static_cast<int>(translation_tables[j].GetSize()); k++) { // цикл по j-й ТП в поисках записи
                if (translation_tables[j].GetRecord(k).raddress == static_cast<RealAddress>(i) && translation_tables[j].GetRecord(k).is_valid == true) {
                    candidate_raddress = static_cast<RealAddress>(i);
                    candidate_process = translation_tables[j].GetProcess();
                    candidate_vaddress = translation_tables[j].GetRecord(k).vaddress;
                    
                    translation_tables[j].GetRecord(k).is_valid = false;
                    found_flag = true;
                    break;
                }
            }
            if (found_flag) {
                break;
            }
        }
        if (found_flag) {
            break;
        }
    }

    // В очередь запросов добавляем новый запрос на загрузку данных в АС из виртуального
    // адреса кандидата
    requester.AddRequest(candidate_process, candidate_vaddress, candidate_raddress, true);

    if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
        Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS->GetScheduler().GetProcess(), Process::Work);
    } else {
        Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS, OS::ChangeQueue);
    }

    Log("Deallocate RA=" + string(4 - to_string(candidate_raddress).length(), '0') + to_string(candidate_raddress) + " " + candidate_process->GetName());
}

TT& OS::FindTT(Process* p_process) {
    for (int i = 0; i < static_cast<int>(translation_tables.size()); i++) {
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



CPU::CPU() {
    SetName("CPU");
}

void CPU::Convert(VirtualAddress vaddress, Process *p_process) {
    TTStruct tmp = g_pOS->FindTT(p_process).GetRecord(vaddress);
    if (tmp.is_valid == false) {
        // Прерывание по отсутствию страницы
        Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS, OS::HandelInterruption, vaddress, tmp.raddress, p_process);
        Log("Translate VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + " -> Interrupt");
        return;
    }
    else {
        // Успешное преобразование
        Log("Translate VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + " -> RA=" + string(4 - to_string(tmp.raddress).length(), '0') + to_string(tmp.raddress));
        
        if (GetTime() < g_pOS->GetScheduler().GetProcess()->GetTimeLimit()) {
            Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS->GetScheduler().GetProcess(), Process::Work);
        } else {
            Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS, OS::ChangeQueue);
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
    for (int i = 0; i < static_cast<int>(OS_DEFAULT_DISKSPACE_SIZE); i++) {
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

AE::AE() {
    SetName("AE");
}

void AE::LoadData() {
    RequestStruct tmp = g_pOS->GetRequester().GetRequest();
    for (int i = 0; i < disk.GetSize(); i++) {
        if (disk.GetDiskAddress(i) == false) {
            disk.SetDiskAddress(i, true);
            SwapIndex.push_back({ tmp.p_process, tmp.vaddress, (DiskAddress)i });
            Log("Save RA=" + string(4 - to_string(tmp.raddress).length(), '0') + to_string(tmp.raddress) + " (" + tmp.p_process->GetName() +" VA=" + string(4 - to_string(tmp.vaddress).length(), '0') + to_string(tmp.vaddress) + ") -> AA=" + string(4 - to_string(i).length(), '0') + to_string(i));
            g_pOS->GetRequester().DeleteRequest();

            Schedule(GetTime() + AE_DEFAULT_TIME_FOR_DATA_IO, this, AE::ProcessRequest);
            return;
        }
    }

    __throw_logic_error("NO FREE DISK SPACE");
}

void AE::PopData() {
    RequestStruct tmp = g_pOS->GetRequester().GetRequest();
    for (int i = 0; i < static_cast<int>(SwapIndex.size()); i++) {
        if (SwapIndex[i].p_process == tmp.p_process && SwapIndex[i].vaddress == tmp.vaddress) {
            SwapIndex.erase(SwapIndex.begin() + i);
            disk.SetDiskAddress(i, false) ;
            
            g_pOS->GetRequester().DeleteRequest();
            Schedule(GetTime() + AE_DEFAULT_TIME_FOR_DATA_IO, this, AE::ProcessRequest);
            
            Log("Pop AA=" + string(4 - to_string(i).length(), '0') + to_string(i) + " (" + tmp.p_process->GetName() + " VA=" + string(4 - to_string(tmp.vaddress).length(), '0') + to_string(tmp.vaddress) + ")");
            return;
        }
    }

    __throw_logic_error("SWAP INDEX RECORD NOT FOUND");
}

void AE::ProcessRequest() {
    if (!g_pOS->GetRequester().IsEmpty()) {
        RequestStruct tmp = g_pOS->GetRequester().GetRequest();
        if (tmp.load_flag) {
            Schedule(GetTime(), this, AE::LoadData);
        }
        else {
            Schedule(GetTime(), this, AE::PopData);
        }
    }
    return;
}

bool AE::IsLoaded(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < static_cast<int>(SwapIndex.size()); i++) {
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
    time_limit = 0;
};

void Process::Work() {
    VirtualAddress vaddress = static_cast<VirtualAddress>(randomizer(requested_memory));

    Schedule(GetTime() + PROCESS_DEFAULT_WORK_TIME, g_pCPU, CPU::Convert, vaddress, this);
}

void Process::Wait() {
    //
}

void Process::Start() {
    Schedule(GetTime(), g_pOS, OS::LoadProcess, this);
    Log("Start!");
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
}