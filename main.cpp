#include <iostream>
#include <stdint.h>
#include <assert.h>
#include "vm.hpp"


using namespace std;

Sim *g_pSim;
OS *g_pOS;
CPU *g_pCPU;
Computer *g_pComputer;
AE *g_pAE;

int main()
{
    g_pSim = new Sim;

    const int amount = 15;

    g_pOS = new OS;
    g_pCPU = new CPU;
    g_pAE = new AE;

    g_pComputer = new Computer;
    g_pComputer->PrintCurrentConfig();

    Process * all_processes[amount];
    for (int i = 0; i < amount; i++) {
        all_processes[i] = new Process;
        string name = "Process" + string(2-to_string(i).length(), '0') + to_string(i);
        all_processes[i]->SetName(name);
        Schedule(g_pSim->GetTime(), all_processes[i], &Process::Start);
    };


    SimulatorTime limit = 72*Hour;
    g_pSim->SetLimit(limit);
    while(!g_pSim->Run())
    {
        PrintTime(&std::cout);
        std::cout << "Do you want to proceed? [Y/n]" << std::endl;

        char c[3];
        std::cin.getline(c,sizeof(c));

        if(c[0] == 'n')
        {
            break;
        };

        g_pSim->SetLimit(g_pSim->GetTime()+limit);
    }

    for (int i = 0; i < amount; i++) {
        delete all_processes[i];
    };

    delete g_pSim;
    delete g_pComputer;
    delete g_pOS;
    delete g_pCPU;
    delete g_pAE;
    return 0;
}
