#include <iostream>
#include "riscVRunByOrder.h"

riscVRunByOrder riscv;

int main() {
    riscv.init();
    riscv.runByOrder();
    return 0;
}
