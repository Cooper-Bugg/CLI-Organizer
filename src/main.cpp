#include "FileOrganizer.h"
#include <iostream>
#include <filesystem> // Include filesystem explicitly

// Explicitly alias the namespace here
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Check for "Info Mode" first (only requires 2 args: ./organizer <path> --info)
    if (argc >= 3 && std::string(argv[2]) == "--info") {
        fs::path target = fs::absolute(argv[1]);
        FileOrganizer organizer;
        organizer.printDriveInfo(target);
        return 0; // Exit after printing info
    }

    // Existing validation for Organize Mode
    if (argc < 3) {
        std::cerr << "Usage:\n";
        std::cerr << "  Organize: " << argv[0] << " <source> <dest> [--run]\n";
        std::cerr << "  Check Drive: " << argv[0] << " <path_on_drive> --info\n";
        return 1;
    }

    fs::path source = fs::absolute(argv[1]);
    fs::path dest = fs::absolute(argv[2]);
    bool dryRun = true;

    // Default is ALWAYS dry-run unless explicit flag is present
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--run") {
            dryRun = false;
        }
    }

    // Safety Invariant: Prevent recursive nesting to avoid infinite loops
    std::string srcStr = source.string();
    std::string destStr = dest.string();
    if (destStr.find(srcStr) == 0 || srcStr.find(destStr) == 0) {
        std::cerr << "[FATAL] Source and Destination cannot be nested inside each other.\n";
        return 1;
    }

    // Safety Invariant: Source must exist and be a directory
    if (!fs::exists(source) || !fs::is_directory(source)) {
        std::cerr << "[FATAL] Source path does not exist or is not a directory.\n";
        return 1;
    }

    FileOrganizer organizer;
    organizer.organize(source, dest, dryRun);

    return 0;
}