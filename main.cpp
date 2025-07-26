#include <iostream>
#include "auth.h"
#include "ls.h"

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string cmd = argv[1];
        if (cmd == "auth") authenticate();
        else if (cmd == "ls") list_drive_items();
        else std::cout << "Unknown command.\n";
    } else {
        std::cout << "Usage:\n  ./onedrivecli auth\n  ./onedrivecli ls\n";
    }
    return 0;
}