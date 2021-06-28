#include <iostream>
#include "binaryManager.h"
int main() {
    binaryManager bm(50);
    std::cout<<bm[4]<<' '<<bm[5]<<' '<<bm[6]<<'\n';
    std::cout<<bm.slice(4,5)<<' '<<bm.slice(0,7);
    return 0;
}
