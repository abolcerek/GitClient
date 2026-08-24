#include "GitClient/repository.hpp"

#include <fstream>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::exit(1);
    }
    const std::string_view first_arg = argv[1];
    if (first_arg == "init") {
        if (GitClient::init_repository(fs::current_path())) {
            std::cout << "Success\n";
            return 0;
        } else {
            std::cout << "Failure\n";
            return 1;
        }
    }
    std::cerr << "Unknown command";
    return 1;
}