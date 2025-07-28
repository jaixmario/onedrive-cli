#include <iostream>
#include "auth.h"
#include "ls.h"

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string command = argv[1];

        if (command == "auth") {
            authenticate();
        } else if (command == "ls") {
            std::string path = (argc >= 3) ? argv[2] : "";
            list_drive_items(path);
        } else {
            std::cerr << "❌ Unknown command\n";
        }
    } else {
        std::cout << "Usage:\n";
        std::cout << "  ./onedrivecli auth         # Login and save tokens\n";
        std::cout << "  ./onedrivecli ls [path]    # List OneDrive files or folder\n";
    }
}