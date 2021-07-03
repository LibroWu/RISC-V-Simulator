//
// Created by Libro on 2021/7/2.
//

#ifndef CODE_OPERATOR_H
#define CODE_OPERATOR_H

enum OpType{
    R,J,S,U,I,B;
};

class baseOperator {
protected:
    unsigned char opcode,func3,func7;
    unsigned int immediate,npc;
    OpType opType;
public:
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2)=0;
    void setValue(unsigned char opCode,unsigned char Func3,unsigned char Func7,unsigned int Immediate,unsigned int Npc,OpType type);
};

class RtypeOperator:public baseOperator{
public:
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

class ItypeOperator:public baseOperator{
public:
    unsigned char getFunc3();
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

class StypeOperator:public baseOperator{
public:
    unsigned char getFunc3();
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

class BtypeOperator:public baseOperator{
public:
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

class JtypeOperator:public baseOperator{
public:
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

class UtypeOperator:public baseOperator{
public:
    virtual void operate(unsigned int& reg_rd,unsigned int reg_rs1,unsigned int reg_rs2) override;
};

#endif //CODE_OPERATOR_H
