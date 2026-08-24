#include "GitClient/repository.hpp"

#include <fstream>

namespace fs = std::filesystem;

namespace GitClient {
    bool init_repository(const fs::path& root) {
        const fs::path gitPath = root / ".git";
        if (fs::exists(gitPath)) {
            return false;
        } 
        const fs::path objectsPath = gitPath / "objects";
        const fs::path refsPath = gitPath / "refs" / "heads";
        const fs::path headPath = gitPath / "HEAD";
        fs::create_directories(objectsPath);
        fs::create_directories(refsPath);
        std::ofstream HEAD(headPath);
        if (!HEAD) {
            return false;
        }
        HEAD << "ref: refs/heads/main\n";
        return true;
    }
}