//
// Created by Libro on 2021/7/2.
//

#include "Operator.h"

void baseOperator::setValue(unsigned char opCode, unsigned char Func3, unsigned char Func7, unsigned int Immediate,
                            unsigned int Npc,OpType type) {
    opcode=opCode,func3=Func3,func7=Func7;
    immediate=Immediate,npc=Npc;
    opType=type;
}

void RtypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    switch (func3) {
        case 0:reg_rd=(func7>>5)?reg_rs1-reg_rs2:reg_rs1+reg_rs2;break;
        case 1:reg_rd=reg_rs1<<reg_rs2;break;
        case 2:reg_rd=int(reg_rs1)<int(reg_rs2);break;
        case 3:reg_rd=reg_rs1<reg_rs2; break;
        case 4:reg_rd=reg_rs1^reg_rs2;break;
        case 5:reg_rd=((func7>>5)?int(reg_rs1):reg_rs1)>>reg_rs2;break;
        case 6:reg_rd=reg_rs1|reg_rs2;break;
        case 7:reg_rd=reg_rs1&reg_rs2;break;
    }
}

void UtypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    switch (opcode) {
        case 55: reg_rd=immediate;break;
        case 23: reg_rd=npc+immediate;npc+=immediate;break;
    }
}

void BtypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    switch (func3) {
        case 0:npc+=(reg_rs1==reg_rs2)?immediate:4;break;
        case 1:npc+=(reg_rs1!=reg_rs2)?immediate:4;break;
        case 4:npc+=(int(reg_rs1)<int(reg_rs2))?immediate:4;break;
        case 5:npc+=(int(reg_rs1)>=int(reg_rs2))?immediate:4;break;
        case 6:npc+=(reg_rs1<reg_rs2)?immediate:4;break;
        case 7:npc+=(reg_rs1>=reg_rs2)?immediate:4;break;
    }
}

void JtypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    reg_rd=npc+4;
    npc+=immediate;
}

void ItypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    unsigned int t;
    switch (opcode) {
        case 3:
            break;
        case 19:
            switch (func3) {
                case 0:reg_rd=reg_rs1+immediate;break;
                case 1:reg_rd=reg_rs1<<immediate;break;
                case 2:reg_rd=int(reg_rs1)<int(immediate);break;
                case 3:reg_rd=reg_rs1<immediate;break;
                case 4:reg_rd=reg_rs1^immediate;break;
                case 5:reg_rd=((func7>>5)?int(reg_rs1):reg_rs1)>>immediate;break;
                case 6:reg_rd=reg_rs1|immediate;break;
                case 7:reg_rd=reg_rs1&immediate;break;
            }
            break;
        case 103:
            t=npc+4;
            npc=(reg_rs1+immediate) & ~1;
            reg_rd=t;
            break;
    }
}

void StypeOperator::operate(unsigned int &reg_rd, unsigned int reg_rs1, unsigned int reg_rs2) {
    
}