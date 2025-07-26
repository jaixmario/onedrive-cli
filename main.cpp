#include <iostream>
#include "auth.h"

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "auth") {
        authenticate();
    } else {
        std::cout << "Usage: ./onedrivecli auth\n";
    }
    return 0;
}