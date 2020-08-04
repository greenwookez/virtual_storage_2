#include "sim.hpp"
#include "vm.hpp"
#include "process.hpp"

class OS : public Agent {
    class TT {
        Process *p_process;

        struct TTStruct {
            VirtualAddress vaddress;
            RealAddress raddress;
            bool is_invalid;
        };

        vector <TTStruct> records;

        public:
        TT();
        ~TT();
        void AddRecord();
        void EditRecord();
    };

    vector <TT> translation_tables;

    public:
    OS();
    ~OS();
    void HandelInterruption();
    void LoadProcess();
    void Allocate();
    void Substitute();

    void Work();
    void Wait();
    void Start();
};