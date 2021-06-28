//
// Created by Libro on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_BINARYMANAGER_H
#define RISC_V_SIMULATOR_BINARYMANAGER_H
#include <string>
class binaryManager {
private:
    unsigned int value;
public:
    binaryManager(unsigned int v);

    binaryManager();

    void setValue(unsigned int v);

    bool operator[](unsigned char pos);

    unsigned int slice(unsigned char st,unsigned char ed);

    operator unsigned int();
};


#endif //RISC_V_SIMULATOR_BINARYMANAGER_H
