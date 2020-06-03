#include <fstream>
#include <ctime>
#include <cstdlib>
#include "fileMaker.h"

void fileMaker::makedata(int& n) {
    std::srand(unsigned(std::time(0)));

    int a[n];
    for (int i = 0; i < n; ++i)
        a[i] = std::rand();

    std::ofstream myFile;
    myFile.open ("data.bin", std::ios:: out | std::ios::binary);

    myFile.write((char*)&a, n*sizeof(int));
    myFile.close();

}