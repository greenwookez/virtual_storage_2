#include "vm.hpp"
#include <iostream>
#include <iomanip>
#include <random>
struct RequestStruct;

extern Sim *g_pSim;
extern OS *g_pOS;
extern CPU *g_pCPU;
extern AE *g_pAE;

void AgentVM :: Log(string text) {
    string tail =
    " || RML=" + to_string(g_pOS->ComputeRML()).substr(0,5) +
    " AEL=" + to_string(g_pAE->ComputeAEL()).substr(0,5) +
    " PSL="  + to_string(g_pAE->ComputePSL()).substr(0,5) +
    " IOTT= " + to_string(g_pAE->io_total_time);
    
    Agent::Log(text + string(60-text.length(), ' ') + tail, CONFIG_LOG_ENABLE_EMPTY_STRINGS);
}

TT::TT(Process* _p_process, PageNumber size) {
    for (int i = 0; i < static_cast<int>(size); i++) {
        TTStruct tmp_struct = { static_cast<VirtualAddress>(i), 0, false };
        records.push_back(tmp_struct);
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
        Schedule(g_pSim->GetTime(), g_pAE, &AE::ProcessRequest);
    }

    string text = "      New request (" + p_process->GetName() + " VA=" 
    + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) 
    + " RA=" + string(4 - to_string(raddress).length(), '0') + to_string(raddress) 
    + " LF=" + to_string(load_flag) + ")";
    PrintTime(&std::cout);
    std::cout << " " << std::setw(10) << std::setfill(' ') << std::right << "Requester"
        << "   " << text << std::endl;
    RequestStruct tmp_struct = { load_flag, p_process, vaddress, raddress };
    request_queue.push_back(tmp_struct);
}

void Requester::DeleteRequest(Process* p_process, VirtualAddress vaddress) {
    for (int i = 0; i < static_cast<int>(request_queue.size()); i++) {
        if (request_queue[i].p_process == p_process && request_queue[i].vaddress == vaddress) {
            
            string text = "      Delete request (" + request_queue[0].p_process->GetName() + " VA=" 
            + string(4 - to_string(request_queue[0].vaddress).length(), '0') + to_string(request_queue[0].vaddress) 
            + " RA=" + string(4 - to_string(request_queue[0].raddress).length(), '0') + to_string(request_queue[0].raddress) 
            + " LF=" + to_string(request_queue[0].load_flag) + ")";
            PrintTime(&std::cout);
            std::cout << " " << std::setw(10) << std::setfill(' ') << std::right << "Requester"
                << "   " << text << std::endl;

            request_queue.erase(request_queue.begin() + i);
            return;
        }
    }

    __throw_logic_error("REQUEST NOT FOUND");
    //request_queue.erase(request_queue.begin());


    
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
        for (int i = 0; i < static_cast<int>(request_queue.size()); i++) {
            std::cout << "{" << request_queue[i].load_flag << "} {" <<
            request_queue[i].p_process << "} {" << request_queue[i].vaddress <<
            "} {" << request_queue[i].raddress << "}" << endl;
        }
    }
}

void Scheduler::AddProcess(Process* p_process) {
    if (IsEmpty()) {
        // Если очередь пуста, планируем событие обработки этой очереди
        Schedule(g_pSim->GetTime(), g_pOS, &OS::ProcessQueue);
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
    scheduler.GetProcess()->SetTimeLimit(OS_DEFAULT_PROCESS_QUEUE_TIME_LIMIT);
    // Планируем саму работу процесса
    Schedule(GetTime(), scheduler.GetProcess(), &Process::Work);
}

void OS::ChangeQueue() {
    scheduler.GetProcess()->SetTimeLimit(0);
    scheduler.PutInTheEnd();
    Schedule(GetTime(), g_pOS, &OS::ProcessQueue);
}

OS::OS() {
    SetName("OS");
}

void OS::HandelInterruption(VirtualAddress vaddress, RealAddress raddress,  Process* p_process) {
    // Если по виртуальному адресу vaddress есть данные в АС, необходимо их
    // выгрузить
    
    if (g_pAE->IsLoaded(p_process, vaddress)) {
        requester.AddRequest(p_process, vaddress, raddress, false);
        
    }
    
    // Ранее здесь Allocate планирвоался, а не вызывался
    Allocate(vaddress, p_process);
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

            Process* scheduler_process = g_pOS->GetScheduler().GetProcess();
            if (scheduler_process->GetTimeLimit() > 0) {
                scheduler_process->SetTimeLimit(scheduler_process->GetTimeLimit() - CPU_DEFAULT_TIME_FOR_CONVERSION);
                Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, scheduler_process, &Process::Work);
            } else {
                Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS, &OS::ChangeQueue);
            }


            Log("  Allocate RA=" + string(4 - to_string(i).length(), '0') + to_string(i) + " as VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + ", resume");
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
    while (!found_flag) {
        candidate_raddress = static_cast<RealAddress>(randomizer(ram.GetSize()));
        for (int j =0; j < static_cast<int>(translation_tables.size()); j++) { // цикл по всем ТП
            for (int k = 0; k < static_cast<int>(translation_tables[j].GetSize()); k++) { // цикл по записям j-й ТП
                if (translation_tables[j].GetRecord(k).raddress == candidate_raddress && translation_tables[j].GetRecord(k).is_valid == true) {
                    translation_tables[j].GetRecord(k).is_valid = false;
                    candidate_process = translation_tables[j].GetProcess();
                    candidate_vaddress = translation_tables[j].GetRecord(k).vaddress;
                    
                    found_flag = true;
                    break;
                }
            }
            if (found_flag) {
                break;
            }
        }
    }
    Log("  Deallocate RA=" + string(4 - to_string(candidate_raddress).length(), '0') + to_string(candidate_raddress) + " " + candidate_process->GetName());
    // В очередь запросов добавляем новый запрос на загрузку данных в АС из виртуального
    // адреса кандидата
    requester.AddRequest(candidate_process, candidate_vaddress, candidate_raddress, true);

    // Вносим изменение в ТП процесса о новом соответствии виртуального адреса
    // реальному
    FindTT(p_process).GetRecord(vaddress).raddress = candidate_raddress;
    // А также о том, что реальный адрес действителен
    FindTT(p_process).GetRecord(vaddress).is_valid = true;

    Process* scheduler_process = g_pOS->GetScheduler().GetProcess();
    if (scheduler_process->GetTimeLimit() > 0) {
        scheduler_process->SetTimeLimit(scheduler_process->GetTimeLimit() - CPU_DEFAULT_TIME_FOR_CONVERSION);
        Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, scheduler_process, &Process::Work);
    } else {
        Schedule(GetTime()+OS_DEFAULT_TIME_FOR_ALLOCATION, g_pOS, &OS::ChangeQueue);
    }
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

float OS::ComputeRML() {
    float result = 0.0;
    for (int i = 0; i < ram.GetSize(); i++) {
        if (ram.GetRealAddress(static_cast<PageNumber>(i)) == true) {
            result++;
        }
    }
    return result/ram.GetSize() * 100.;
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
    Process* scheduler_process = g_pOS->GetScheduler().GetProcess();
    if (tmp.is_valid == false) {
        // Прерывание по отсутствию страницы
        //scheduler_process->SetTimeLimit(scheduler_process->GetTimeLimit() - CPU_DEFAULT_TIME_FOR_CONVERSION);
        Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS, &OS::HandelInterruption, vaddress, tmp.raddress, p_process);
        Log("Translate VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + " -> Interrupt");
        return;
    } else {
        // Успешное преобразование
        Log("Translate VA=" + string(4 - to_string(vaddress).length(), '0') + to_string(vaddress) + " " + p_process->GetName() + " -> RA=" + string(4 - to_string(tmp.raddress).length(), '0') + to_string(tmp.raddress) + " TL=" + to_string(p_process->GetTimeLimit()));
        
        if (scheduler_process->GetTimeLimit() > 0) {
            scheduler_process->SetTimeLimit(scheduler_process->GetTimeLimit() - CPU_DEFAULT_TIME_FOR_CONVERSION);
            Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, scheduler_process, &Process::Work);
        } else {
            Schedule(GetTime() + CPU_DEFAULT_TIME_FOR_CONVERSION, g_pOS, &OS::ChangeQueue);
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
    for (int i = 0; i < static_cast<int>(AE_DEFAULT_DISKSPACE_SIZE); i++) {
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
    io_total_time = 0;
    SetName("AE");
}

void AE::LoadData() {
    RequestStruct tmp = g_pOS->GetRequester().GetRequest();
    for (int i = 0; i < disk.GetSize(); i++) {
        if (disk.GetDiskAddress(i) == false) {
            disk.SetDiskAddress(i, true);
            SwapIndexStruct tmp_struct = {tmp.p_process, tmp.vaddress, static_cast<DiskAddress>(i)};
            SwapIndex.push_back(tmp_struct);
            io_total_time += AE_DEFAULT_TIME_FOR_DATA_IO;
            Log("    Save RA=" + string(4 - to_string(tmp.raddress).length(), '0') + to_string(tmp.raddress) + " (" + tmp.p_process->GetName() +" VA=" + string(4 - to_string(tmp.vaddress).length(), '0') + to_string(tmp.vaddress) + ") -> AA=" + string(4 - to_string(i).length(), '0') + to_string(i));
            g_pOS->GetRequester().DeleteRequest(tmp.p_process, tmp.vaddress);

            Schedule(GetTime() + AE_DEFAULT_TIME_FOR_DATA_IO, this, &AE::ProcessRequest);
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
            disk.SetDiskAddress(SwapIndex[i].daddress, false);
            io_total_time += AE_DEFAULT_TIME_FOR_DATA_IO;
            Log("    Pop AA=" + string(4 - to_string(i).length(), '0') + to_string(i) + " (" + tmp.p_process->GetName() + " VA=" + string(4 - to_string(tmp.vaddress).length(), '0') + to_string(tmp.vaddress) + ")");            
            g_pOS->GetRequester().DeleteRequest(tmp.p_process, tmp.vaddress);
            Schedule(GetTime() + AE_DEFAULT_TIME_FOR_DATA_IO, this, &AE::ProcessRequest);
            
            return;
        }
    }

    __throw_logic_error("SWAP INDEX RECORD NOT FOUND");
}

void AE::ProcessRequest() {
    if (!g_pOS->GetRequester().IsEmpty()) {
        RequestStruct tmp = g_pOS->GetRequester().GetRequest();
        
        string text = "      Process request (" + tmp.p_process->GetName() + " VA=" + string(4 - to_string(tmp.vaddress).length(), '0') + to_string(tmp.vaddress) + " RA=" + string(4 - to_string(tmp.raddress).length(), '0') + to_string(tmp.raddress) + " LF=" + to_string(tmp.load_flag) + ")";
        PrintTime(&std::cout);
        std::cout << " " << std::setw(10) << std::setfill(' ') << std::right << "Requester"
            << "   " << text << std::endl;

        if (tmp.load_flag) {
            //Schedule(GetTime(), this, AE::LoadData);
            LoadData();
        }
        else {
            //Schedule(GetTime(), this, AE::PopData);
            PopData();
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
}

float AE::ComputeAEL() {
    float result =0.0;
    for (int i = 0; i < disk.GetSize(); i++) {
        if (disk.GetDiskAddress(static_cast<PageNumber>(i)) == true) {
            result++;
        }
    }
    return result/disk.GetSize() * 100.;
}

double AE::ComputePSL() {
    return (float)io_total_time / (float)g_pSim->GetTime() * 100.;
}


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
}

void Process::Work() {
    if (randomizer(100) + 1 >= 30) {
        // Протолкнуть время симуляции вручную?? Ведь процесс совершил работу
        g_pSim->GetTime() = g_pSim->GetTime() + PROCESS_DEFAULT_WORK_TIME;
        Log("Working. No need of CPU.");
        if (GetTimeLimit() > 0) {
            SetTimeLimit(GetTimeLimit() - PROCESS_DEFAULT_WORK_TIME);
            Schedule(GetTime(), this, &Process::Work);
        } else {
            Schedule(GetTime(), g_pOS, &OS::ChangeQueue);
        }
    } else {
        VirtualAddress vaddress = static_cast<VirtualAddress>(randomizer(requested_memory));
        Schedule(GetTime(), g_pCPU, &CPU::Convert, vaddress, this);
    }
}

void Process::Wait() {
    //
}

void Process::Start() {
    Schedule(GetTime(), g_pOS, &OS::LoadProcess, this);
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
    std::random_device rd; 
    std::mt19937 mersenne(rd());
    return mersenne() % max;
}
