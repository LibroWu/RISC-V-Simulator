//
// Created by Libro on 2021/6/28.
//

#include "binaryManager.h"

void binaryManager::setValue(unsigned v) {
    value=v;
}

binaryManager::binaryManager():value(0) {}

binaryManager::binaryManager(unsigned int v):value(v) {}

bool binaryManager::operator[](unsigned char pos) {
    return value&(1ul<<pos);
}

unsigned int binaryManager::slice(unsigned char st, unsigned char ed) {
    if (st>ed) return 0;
    return ((((1ul<<(ed-st+1))-1)<<st) & value)>>st;
}

binaryManager::operator unsigned int() {
    return value;
}