#include <iostream>
#include <string>
int main(int argc, char *argv[]){
    if (argc != 2){
       std::cout << "invalid execution format\n";
       std::cout << "expected: <executable name> <file VAAWEMBLY address>\n";
       return 1;
    }
    std::string file_name(argv[1]);
}