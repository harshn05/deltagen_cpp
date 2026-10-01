# deltagen

**`deltagen`** is a high-performance, standalone C++17 CLI tool that tracks structural and content differences (`ADD`, `MODIFY`, `DELETE`) between two directories using Windows CryptoAPI SHA-256 hashing and BFS-based depth control to generate a lightweight JSON manifest.

---

## 🚀 Features

* **Precise Delta Tracking:** Categorizes all folder differences into `ADD`, `MODIFY`, and `DELETE` actions.
* **Content Hash Verification:** Uses Windows CryptoAPI SHA-256 hashing to verify file contents reliably without third-party hash dependencies.
* **BFS Depth Control:** Uses Breadth-First Search directory traversal for accurate level-by-level recursion limits (`--depth`).
* **Flexible CLI Parsing:** Supports both standard command-line argument formats (`--flag value` and `--flag=value`).
* **Zero External DLL Dependencies:** Statically linked binary for seamless deployment on any target system.
* **Cross-Toolchain CMake Setup:** Native support for MSVC, `clang-cl`, and MinGW-w64.

---

## 🛠️ Requirements

* C++17 compliant compiler (MSVC, Clang, or GCC / MinGW-w64)
* CMake 3.15 or higher
* [`nlohmann/json`](https://github.com/nlohmann/json) single-header library (`json.hpp`) inside the `include/` directory
* *(Optional)* Doxygen for generating HTML API documentation

---

## 📁 Repository Structure

```text
deltagen/
├── CMakeLists.txt
├── include/
│   └── json.hpp
└── src/
    └── main.cpp
