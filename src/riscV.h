//
// Created by Libro on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_RISCV_H
#define RISC_V_SIMULATOR_RISCV_H
#include "binaryManager.h"
#include <cstring>
#include <iostream>
class riscV {
private:
    binaryManager command;
    unsigned int memory[500000],cache[32],pc;
public:
    riscV();

    //put data into memory
    void init();

    void resetPC();

    void runCommand();

    void runByOrder();
};


#endif //RISC_V_SIMULATOR_RISCV_H
