#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

// Helper to ensure directories exist
void create_directory_if_missing(const std::string& path) {
    // Strips file name to get directory path if a file path is passed
    size_t last_slash = path.find_last_of("/");
    if (last_slash != std::string::npos) {
        std::string dir = path.substr(0, last_slash);
        // Create directory (mkdir returns 0 on success, or -1 if it exists)
        mkdir(dir.c_str(), 0777); 
    }
}

int main() {
    // 1. Define your exact paths from the log
    std::string src_path = "var/www/storage/body_42952_0";
    std::string dst_path = "var/www/upload/42952_0.txt";

    std::cout << "=== STARTING ISOLATED RENAME TEST ===" << std::endl;

    // 2. Setup the environment (Create folders and dummy source file)
    create_directory_if_missing(src_path);
    create_directory_if_missing(dst_path);

    std::ofstream dummy_file(src_path.c_str());
    if (!dummy_file) {
        std::cerr << "[-] Fail: Could not create dummy source file at: " << src_path 
                  << " Reason: " << std::strerror(errno) << std::endl;
        return 1;
    }
    dummy_file << "This is some dummy web server request body data.\n";
    dummy_file.close(); // Crucial: Close it so the OS releases any locks!

    std::cout << "[+] Created dummy source file successfully." << std::endl;
    std::cout << "[*] Attempting std::rename..." << std::endl;

    // 3. The actual test
    if (std::rename(src_path.c_str(), dst_path.c_str()) == 0) {
        std::cout << "[SUCCESS] Code works fine! File renamed successfully." << std::endl;
        // Clean up the successful move
        std::remove(dst_path.c_str());
    } 
    else {
        int error_code = errno;
        std::cout << "[FAILURE] std::rename failed!" << std::endl;
        std::cout << "  -> Error Code: " << error_code << std::endl;
        std::cout << "  -> Description: " << std::strerror(error_code) << std::endl;

        if (error_code == EXDEV) {
            std::cout << "\n[DIAGNOSIS]: EXDEV detected. This confirms WSL is trying to "
                      << "move the file across different virtual filesystems/mounts. "
                      << "You must use a copy-and-delete fallback approach." << std::endl;
        } else if (error_code == EACCES || error_code == EPERM) {
            std::cout << "\n[DIAGNOSIS]: Permission issue. Your WSL user doesn't "
                      << "have write privileges in one of these directories." << std::endl;
        } else if (error_code == ENOENT) {
            std::cout << "\n[DIAGNOSIS]: Folder path doesn't exist. Check your relative path resolution." << std::endl;
        }

        // Clean up dummy file
        std::remove(src_path.c_str());
    }

    return 0;
}