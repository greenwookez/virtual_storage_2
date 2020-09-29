#include <iostream>
#include <stdint.h>
#include <assert.h>
#include "vm.hpp"


using namespace std;

Sim *g_pSim;
OS *g_pOS;
CPU *g_pCPU;
AE *g_pAE;


extern const SimulatorTime CONFIG_SIM_TIME_LIMIT;
int main()
{
    try {
        g_pSim = new Sim;

        const int amount = 15;

        g_pOS = new OS;
        g_pCPU = new CPU;
        g_pAE = new AE;

        Process * all_processes[amount];
        for (int i = 0; i < amount; i++) {
            all_processes[i] = new Process;
            string name = "Process" + string(2-to_string(i).length(), '0') + to_string(i);
            all_processes[i]->SetName(name);
            Schedule(g_pSim->GetTime(), all_processes[i], &Process::Start);
        };

        g_pSim->SetLimit(CONFIG_SIM_TIME_LIMIT);
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

            g_pSim->SetLimit(g_pSim->GetTime()+CONFIG_SIM_TIME_LIMIT);
        }

        for (int i = 0; i < amount; i++) {
            
            for (int j = 0; j < g_pOS->FindTT(all_processes[i]).GetSize(); j++) {
                if (g_pOS->FindTT(all_processes[i]).GetRecord(j).is_valid == false) {
                    cout << all_processes[i]->GetName() << " vaddress " << g_pOS->FindTT(all_processes[i]).GetRecord(j).vaddress << " is invalid." << endl;
                }
            }

            delete all_processes[i];
        }


        int count2 = 0;
        for (int i = 0; i < g_pOS->ram.GetSize(); i++) {
            if (g_pOS->ram.GetRealAddress(i) == false) {
                count2++;
            }
        }
        cout << "Number of free pages of RAM: " << count2 << endl;

        delete g_pSim;
        delete g_pOS;
        delete g_pCPU;
        delete g_pAE;
    }
    catch(exception& ex) {
        cout << ex.what() << endl;
    }
    catch(Requester& err) {
        cout << "NO INFO IN TTS FOUND FOR CANDIDATE" << endl;
        err.PrintQueue();
    }

    cout << "Returns 0" << endl;
    return 0;
}
