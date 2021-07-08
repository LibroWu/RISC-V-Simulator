//
// Created by Libro on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_RISCV_H
#define RISC_V_SIMULATOR_RISCV_H
#include "binaryManager.h"
#include <cstring>
#include <iostream>
class riscVRunByOrder {
private:
    binaryManager command;
    unsigned int reg[32],pc;
    unsigned char memory[500000];

    unsigned int combineChars(int pos=-1,unsigned char len=4);

    unsigned int reg_count = 0;
public:
    riscVRunByOrder();

    //put data into memory
    void init();

    void resetPC();

    void runCommand();

    void runByOrder();
};


#endif //RISC_V_SIMULATOR_RISCV_H
