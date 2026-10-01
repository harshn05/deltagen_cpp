/**
 * @file main.cpp
 * @brief Directory comparison tool (deltagen) to generate JSON manifests of file differences.
 * @author Harsh Kumar Narula
 * @date 2026
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <queue>

#include "json.hpp"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

/**
 * @brief Calculates the SHA-256 hash of a file using Windows CryptoAPI.
 * 
 * @param filepath Path to the file to be hashed.
 * @return std::string Hexadecimal string representation of SHA-256 hash, or empty string on failure.
 */
std::string get_file_sha256(const fs::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return "";

#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    char buffer[65536];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (!CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer), static_cast<DWORD>(file.gcount()), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    DWORD hash_len = 32;
    BYTE hash[32];
    std::string hash_hex = "";

    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hash_len, 0)) {
        std::ostringstream ss;
        for (DWORD i = 0; i < hash_len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        hash_hex = ss.str();
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hash_hex;
#else
    return "hash_not_implemented_for_posix";
#endif
}

/**
 * @brief Scans a directory up to a specified recursion depth using BFS traversal.
 * 
 * @param base_dir Base root directory path to scan.
 * @param max_depth Maximum recursion depth allowed (-1 for unlimited depth, 0 for root folder only).
 * @return std::unordered_map<std::string, fs::path> Map of relative file path string to full path.
 */
std::unordered_map<std::string, fs::path> scan_directory(const fs::path& base_dir, int max_depth) {
    std::unordered_map<std::string, fs::path> file_map;

    if (!fs::exists(base_dir) || !fs::is_directory(base_dir)) {
        return file_map;
    }

    // BFS queue storing pair of directory path and its relative depth level
    std::queue<std::pair<fs::path, int>> dir_queue;
    dir_queue.push({base_dir, 0});

    while (!dir_queue.empty()) {
        auto [current_dir, current_depth] = dir_queue.front();
        dir_queue.pop();

        std::error_code ec;
        fs::directory_iterator it(current_dir, fs::directory_options::skip_permission_denied, ec);
        fs::directory_iterator end;

        if (ec) continue;

        for (; it != end; ++it) {
            const auto& entry = *it;
            
            if (fs::is_regular_file(entry.path())) {
                fs::path rel_path = fs::relative(entry.path(), base_dir);
                file_map[rel_path.generic_string()] = entry.path();
            } 
            else if (fs::is_directory(entry.path())) {
                if (max_depth < 0 || current_depth < max_depth) {
                    dir_queue.push({entry.path(), current_depth + 1});
                }
            }
        }
    }

    return file_map;
}

/**
 * @brief Entry point for the deltagen CLI application.
 * 
 * @param argc Command-line argument count.
 * @param argv Command-line argument array.
 * @return int 0 on success, non-zero on error.
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: deltagen <dir_a> <dir_b> [--depth N | --depth=N] [--json manifest.json | --json=manifest.json]\n";
        return 1;
    }

    fs::path dir_a = argv[1];
    fs::path dir_b = argv[2];
    int max_depth = -1;
    std::string json_output = "manifest.json";

    // Flexible CLI parsing for both '--flag value' and '--flag=value' formats
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--depth=", 0) == 0) {
            max_depth = std::stoi(arg.substr(8));
        } else if (arg == "--depth" && i + 1 < argc) {
            max_depth = std::stoi(argv[++i]);
        } else if (arg.rfind("--json=", 0) == 0) {
            json_output = arg.substr(7);
        } else if (arg == "--json" && i + 1 < argc) {
            json_output = argv[++i];
        }
    }

    std::cout << "Scanning Directory A: " << dir_a << " (depth=" << max_depth << ")...\n";
    auto files_a = scan_directory(dir_a, max_depth);
    std::cout << "Total files in A: " << files_a.size() << "\n";

    std::cout << "Scanning Directory B: " << dir_b << " (depth=" << max_depth << ")...\n";
    auto files_b = scan_directory(dir_b, max_depth);
    std::cout << "Total files in B: " << files_b.size() << "\n";

    json manifest = json::array();

    // Check ADD & MODIFY actions
    for (const auto& [rel_path, path_b] : files_b) {
        auto it_a = files_a.find(rel_path);
        if (it_a == files_a.end()) {
            std::cout << "[ADD] " << rel_path << "\n";
            manifest.push_back({{"type", "ADD"}, {"path", rel_path}});
        } else {
            std::string hash_a = get_file_sha256(it_a->second);
            std::string hash_b = get_file_sha256(path_b);

            if (!hash_a.empty() && hash_a != hash_b) {
                std::cout << "[MODIFY] " << rel_path << "\n";
                manifest.push_back({{"type", "MODIFY"}, {"path", rel_path}});
            }
        }
    }

    // Check DELETE actions
    for (const auto& [rel_path, path_a] : files_a) {
        if (files_b.find(rel_path) == files_b.end()) {
            std::cout << "[DELETE] " << rel_path << "\n";
            manifest.push_back({{"type", "DELETE"}, {"path", rel_path}});
        }
    }

    std::ofstream out_file(json_output);
    out_file << manifest.dump(2);
    out_file.close();

    std::cout << "\nManifest saved to " << json_output << " (" << manifest.size() << " changes detected)\n";

    return 0;
}