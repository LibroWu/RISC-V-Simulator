#include <iostream>
#include "riscV.h"

riscV riscv;

int main() {
    riscv.init();
    riscv.runByOrder();
    return 0;
}
