
#include <fstream>
#include <iostream>
#include <ostream>

#include "ZipFile.h"

int main(int argc, char* argv[])
{
    ZipFile zip_file;
    zip_file.add_file("someData.txt");
    zip_file.write("someData.zip");
    ZipFile zip_file2("someData.zip");
    
    return 0;
}
