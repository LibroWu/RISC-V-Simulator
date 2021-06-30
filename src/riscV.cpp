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
unsigned char hexToDec(char c){
    return ((c<='9'&&c>='0')?c-'0':c-'A'+10);
}

void riscV::init() {
    std::string s;
    unsigned int pos=0;
    while (std::cin>>s) {
        if (s=="#") break;
        if (s[0]=='@') {
            pos=0;
            for (int i = 1; i <= 8; ++i)
                pos=(pos<<4)+ hexToDec(s[i]);
        } else {
            memory[pos++]= (hexToDec(s[0])<<4)+ hexToDec(s[1]);
        }
    }
}

riscV::riscV():pc(0) {
    memset(reg, 0, sizeof(reg));
    memset(memory,0,sizeof(memory));
}

unsigned int riscV::combineChars(int pos,unsigned char len) {
    unsigned int res=0;
    if (pos==-1) pos=pc;
    for (int i = pos+len-1; i >= pos; --i) {
        res=(res<<8)+memory[i];
    }
    return res;
}

void riscV::runCommand() {
    if (combineChars()==267388179) {
        std::cout<<((reg[10]) & 255u);
        exit(0);
    }
    command.setValue(combineChars());
    //run the command
    unsigned int immediate,t,sub_t;
    unsigned int st;
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
            pc +=immediate;
            if (command.slice(7, 11)) reg[command.slice(7, 11)]= pc ;
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
                st= reg[command.slice(15, 19)] + immediate;
                switch (command.slice(12, 14)) {
                    case 0:
                        //LB
                        t = memory[st];
                        reg[command.slice(7, 11)] = ((t & (1 << 7)) * ((1 << 24) - 1)<<1) + t;
                        break;
                    case 1:
                        //LH
                        t= combineChars(st,2);
                        reg[command.slice(7, 11)] = ((t & (1 << 15)) * ((1 << 16) - 1)<<1) + t;
                        break;
                    case 2:
                        //LW
                        reg[command.slice(7, 11)] = combineChars(st,4);
                        break;
                    case 4:
                        //LBU
                        reg[command.slice(7, 11)] =memory[st];
                        break;
                    case 5:
                        //LHU
                        reg[command.slice(7, 11)] = combineChars(st,2);
                        break;
                }
            }
            pc+=4;
            break;
        case 35:
            //S-type
            immediate=(command[31]*((1<<21)-1)<<11)+(command.slice(25,30)<<5)+(command.slice(8,11)<<1)+command[7];
            st= reg[command.slice(15, 19)] + immediate;
            switch (command.slice(12,14)) {
                case 0:
                    //SB
                    memory[st]=(reg[command.slice(20, 24)] & 0b11111111);
                    break;
                case 1:
                    //SH
                    t=reg[command.slice(20, 24)];
                    for (int i = st; i < st+2; ++i) {
                        memory[i]=t& 0b11111111;
                        t>>=8;
                    }
                    break;
                case 2:
                    //SW
                    t=reg[command.slice(20, 24)];
                    for (int i = st; i < st+4; ++i) {
                        memory[i]=t & 0b11111111;
                        t>>=8;
                    }
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
                        reg[command.slice(7, 11)] =reg[command.slice(15,19)]<< command.slice(20, 24);
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