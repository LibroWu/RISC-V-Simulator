//
// Created by Libro on 2021/6/28.
//

#include "riscV.h"

void riscV::init() {}

riscV::riscV():pc(0) {
    memset(cache,0,sizeof(cache));
    memset(memory,0,sizeof(memory));
}

void riscV::runCommand() {
    //read the command from memory

    //run the command
        //decrypt the command type
        //turn to the immediate
    //move the pc
}