#include <iostream>
#include "auth.h"
#include "ls.h"
#include "link.h"
#include "dl.h"

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string command = argv[1];

        if (command == "auth") {
            authenticate();
        } else if (command == "ls") {
            std::string path = (argc >= 3) ? argv[2] : "";
            list_drive_items(path);
        } else if (command == "link" && argc >= 3) {
            generate_shareable_link(argv[2]);
        } else if (command == "dl" && argc >= 3) {
            generate_download_url(argv[2]);
        } else {
            std::cerr << "❌ Unknown command\n";
        }
    } else {
        std::cout << "Usage:\n";
        std::cout << "  ./onedrivecli auth           # Login and save tokens\n";
        std::cout << "  ./onedrivecli ls [path]      # List OneDrive folder contents\n";
        std::cout << "  ./onedrivecli link [path]    # Generate public share link\n";
        std::cout << "  ./onedrivecli dl [path]      # Get direct download URL\n";
    }
}