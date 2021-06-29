//
// Created by Libro on 2021/6/28.
//

#include "riscV.h"

void hex(int v,int k){
    if (k==0) {
        std::cout<<'\n';
        return;
    }
    int t=v%16;
    hex(v>>4,k-1);
    std::cout<<((t<=9)?char(t+'0'):char('A'+t-10));
}

void riscV::init() {
    char ch;
    char carr[8];
    unsigned int pos,count=0;
    while (ch=getchar()){
        //debugging
        if (ch=='#') break;
        if (ch=='@') {
            if (!count) {
                for (int i = count-1; i >= 0; --i)
                    memory[pos]= (memory[pos]<<4) + ((carr[i] <= '9' && carr[i] >= '0') ? carr[i] - '0' : carr[i] - 'A'+10);
                pos+=1;
                count=0;
            }
            pos = 0;
            char c;
            for (int i = 0; i < 8; ++i) {
                c = getchar();
                pos = (pos << 4) + ((c <= '9' && c >= '0') ? c - '0' : c - 'A'+10);
            }
            pos>>=2;
        } else if (ch!=' '&& ch!='\n' && ch!='\r'){
            carr[count++]=ch;
            if (!(count&1)) {
                carr[count-2]^=carr[count-1];
                carr[count-1]^=carr[count-2];
                carr[count-2]^=carr[count-1];
            }
            if (count==8) {
                for (int i = 7; i >= 0; --i)
                    memory[pos]= (memory[pos]<<4) + ((carr[i] <= '9' && carr[i] >= '0') ? carr[i] - '0' : carr[i] - 'A'+10);
                pos+=1;
                count=0;
            }
        }
    }
    if (!count) {
        for (int i = count-1; i >= 0; --i)
            memory[pos]= (memory[pos]<<4) + ((carr[i] <= '9' && carr[i] >= '0') ? carr[i] - '0' : carr[i] - 'A'+10);
        pos+=1;
        count=0;
    }
}

riscV::riscV():pc(0) {
    memset(reg, 0, sizeof(reg));
    memset(memory,0,sizeof(memory));
}

void riscV::runCommand() {
    command.setValue(memory[pc>>2]);
    if (pc==4692)
        std::cout<<"asasdawe2";
    if (memory[pc>>2]==267388179) {
        std::cout<<((reg[10]) & 255u);
        exit(0);
    }
    hex(pc,4);
    //run the command
    unsigned int immediate,t,sub_t;
    int st;
    switch (command.slice(0,6)) {
        case 55:
            //LUI U-type
            immediate=command.slice(12,31)<<12;
            if (command.slice(7, 11))reg[command.slice(7, 11)]=immediate;
            pc+=4;
            break;
        case 23:
            //AUIPC U-type
            immediate=command.slice(12,31)<<12;
            pc+=4;
            if (command.slice(7, 11)) reg[command.slice(7, 11)]= pc + immediate;
            break;
        case 111:
            //JAL J-type
            immediate=(command[31]*((1<<12)-1)<<20)+(command.slice(12,19)<<12)+(command[20]<<11)+(command.slice(25,30)<<5)+(command.slice(21,24)<<1);
            if (command.slice(7, 11))reg[command.slice(7, 11)]= pc + 4;
            pc+=immediate;
            break;
        case 103:
            //JALR I-type
            immediate=(command[31]*((1<<21)-1)<<11)+command.slice(20,30);
            t=pc+4;
            pc= (reg[command.slice(15, 19)] + immediate) & ~1;
            if (command.slice(7, 11))reg[command.slice(7, 11)]=t;
            break;
        case 99:
            //B-type
            immediate=(command[31]*((1<<20)-1)<<12)+(command[7]<<11)+(command.slice(25,30)<<5)+(command.slice(8,11)<<1);
            switch (command.slice(12,14)) {
                case 0:
                    //BEQ
                    if (reg[command.slice(15, 19)] == reg[command.slice(20, 24)])
                        pc+=immediate;
                    else pc+=4;
                    break;
                case 1:
                    //BNE
                    if (reg[command.slice(15, 19)] != reg[command.slice(20, 24)])
                        pc+=immediate;
                    else pc+=4;
                    break;
                case 4:
                    //BLT
                    if (int(reg[command.slice(15, 19)]) < int(reg[command.slice(20, 24)]))
                        pc+=immediate;
                    else pc+=4;
                    break;
                case 5:
                    //BGE
                    if (int(reg[command.slice(15, 19)]) >= int(reg[command.slice(20, 24)]))
                        pc+=immediate;
                    else pc+=4;
                    break;
                case 6:
                    //BLTU
                    if (reg[command.slice(15, 19)] < reg[command.slice(20, 24)])
                        pc+=immediate;
                    else pc+=4;
                    break;
                case 7:
                    //BGEU
                    if (reg[command.slice(15, 19)] >= reg[command.slice(20, 24)])
                        pc+=immediate;
                    else pc+=4;
                    break;
            }
            break;
        case 3:
            //I-type
            if (command.slice(7,11)) {
                immediate = (command[31] * ((1 << 21) - 1) << 11) + command.slice(20, 30);
                switch (command.slice(12, 14)) {
                    case 0:
                        //LB
                        t = memory[(reg[command.slice(15, 19)] + immediate) >> 2] & ((1 << 8) - 1);
                        reg[command.slice(7, 11)] = ((t & (1 << 7)) * ((1 << 24) - 1)<<1) + t;
                        break;
                    case 1:
                        //LH
                        t = memory[(reg[command.slice(15, 19)] + immediate) >> 2] & ((1 << 16) - 1);
                        reg[command.slice(7, 11)] = ((t & (1 << 15)) * ((1 << 16) - 1)<<1) + t;
                        break;
                    case 2:
                        //LW
                        reg[command.slice(7, 11)] = memory[(reg[command.slice(15, 19)] + immediate) >> 2];
                        break;
                    case 4:
                        //LBU
                        reg[command.slice(7, 11)] =
                                memory[(reg[command.slice(15, 19)] + immediate) >> 2] & ((1 << 8) - 1);
                        break;
                    case 5:
                        //LHU
                        reg[command.slice(7, 11)] =
                                memory[(reg[command.slice(15, 19)] + immediate) >> 2] & ((1 << 16) - 1);
                        break;
                }
            }
            pc+=4;
            break;
        case 35:
            //S-type
            immediate=(command[31]*((1<<21)-1)<<11)+(command.slice(25,30)<<5)+(command.slice(8,11)<<1)+command[7];
            st= reg[command.slice(15, 19)] + immediate;
            sub_t=(st&0b11)<<3;
            switch (command.slice(12,14)) {
                case 0:
                    //SB
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=(reg[command.slice(20, 24)] & 0b11111111) << sub_t;
                    break;
                case 1:
                    //SH
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=(reg[command.slice(20, 24)] & 0b11111111) << sub_t;
                    sub_t+=8;
                    if (sub_t==32) {sub_t=0;t+=4;}
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=((reg[command.slice(20, 24)] >> 8) & 0b11111111) << sub_t;
                    break;
                case 2:
                    //SW
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=(reg[command.slice(20, 24)] & 0b11111111) << sub_t;
                    sub_t+=8;
                    if (sub_t==32) {sub_t=0;t+=4;}
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=((reg[command.slice(20, 24)] >> 8) & 0b11111111) << sub_t;
                    sub_t+=8;
                    if (sub_t==32) {sub_t=0;t+=4;}
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=((reg[command.slice(20, 24)] >> 16) & 0b11111111) << sub_t;
                    sub_t+=8;
                    if (sub_t==32) {sub_t=0;t+=4;}
                    memory[st>>2]&=~(0b11111111<<sub_t);
                    memory[st>>2]^=((reg[command.slice(20, 24)] >> 24) & 0b11111111) << sub_t;
                    break;
            }
            pc+=4;
            break;
        case 19:
            //I-type
            if (command.slice(7,11)) {
                immediate = (command[31] * ((1 << 21) - 1) << 11) + command.slice(20, 30);
                switch (command.slice(12, 14)) {
                    case 0:
                        //ADDI
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] + immediate;
                        break;
                    case 1:
                        //SLLI
                        reg[command.slice(7, 11)] <<= command.slice(20, 24);
                        break;
                    case 2:
                        //SLTI
                        reg[command.slice(7, 11)] = int(reg[command.slice(15, 19)]) < int(immediate);
                        break;
                    case 3:
                        //SLTIU
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] < immediate;
                        break;
                    case 4:
                        //XORI
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] ^ immediate;
                        break;
                    case 5:
                        if (command[30]) {
                            //SRAI
                            reg[command.slice(7, 11)] = int(reg[command.slice(7, 11)]) >> command.slice(20, 24);
                        } else {
                            //SRLI
                            reg[command.slice(7, 11)] = reg[command.slice(7, 11)] >> command.slice(20, 24);
                        }
                        break;
                    case 6:
                        //ORI
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] | immediate;
                        break;
                    case 7:
                        //ANDI
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] & immediate;
                        break;
                }
            }
            pc+=4;
            break;
        case 51:
            //R-type
            if (command.slice(7,11)) {
                switch (command.slice(12, 14)) {
                    case 0:
                        if (command[30]) {
                            //SUB
                            reg[command.slice(7, 11)] = reg[command.slice(15, 19)] - reg[command.slice(20, 24)];
                        } else {
                            //ADD
                            reg[command.slice(7, 11)] = reg[command.slice(15, 19)] + reg[command.slice(20, 24)];
                        }
                        break;
                    case 1:
                        //SLL
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] << reg[command.slice(20, 24)];
                        break;
                    case 2:
                        //SLT
                        reg[command.slice(7, 11)] = int(reg[command.slice(15, 19)]) < int(reg[command.slice(20, 24)]);
                        break;
                    case 3:
                        //SLTU
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] < reg[command.slice(20, 24)];
                        break;
                    case 4:
                        //XOR
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] ^ reg[command.slice(20, 24)];
                        break;
                    case 5:
                        if (command[30]) {
                            //SRA
                            reg[command.slice(7, 11)] = int(reg[command.slice(15, 19)]) >> reg[command.slice(20, 24)];
                        } else {
                            //SRL
                            reg[command.slice(7, 11)] = reg[command.slice(15, 19)] >> reg[command.slice(20, 24)];
                        }
                        break;
                    case 6:
                        //OR
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] | reg[command.slice(20, 24)];
                        break;
                    case 7:
                        //AND
                        reg[command.slice(7, 11)] = reg[command.slice(15, 19)] & reg[command.slice(20, 24)];
                        break;
                }
            }
            pc+=4;
            break;
    }
}

void riscV::runByOrder() {
    while (1) runCommand();
}