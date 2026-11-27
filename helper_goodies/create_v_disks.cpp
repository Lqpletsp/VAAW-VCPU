#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

void create_v_disk(long size, char *name, char *location) {
  if (size < 0 || name == nullptr || location == nullptr) {
    std::cerr << "Invalid arguments to create_v_disk.\n";
    return;
  }

  fs::path full_path = fs::path(location) / name;

  // Ensure the target directory exists if non-empty
  if (full_path.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(full_path.parent_path(), ec);
    if (ec) {
      std::cerr << "Failed to create directory '"
                << full_path.parent_path().string() << "': " << ec.message()
                << "\n";
      return;
    }
  }

  std::ofstream out(full_path, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    std::cerr << "Failed to open file '" << full_path.string()
              << "' for writing.\n";
    return;
  }

  constexpr std::size_t CHUNK_SIZE = 64 * 1024;
  std::vector<char> buffer(CHUNK_SIZE, 0);

  long remaining = size;
  while (remaining > 0) {
    const std::size_t to_write =
        static_cast<std::size_t>(remaining < static_cast<long>(CHUNK_SIZE)
                                     ? remaining
                                     : static_cast<long>(CHUNK_SIZE));
    out.write(buffer.data(), static_cast<std::streamsize>(to_write));
    if (!out) {
      std::cerr << "Write error while creating virtual disk.\n";
      return;
    }
    remaining -= static_cast<long>(to_write);
  }

  std::cout << "Created virtual disk at '" << full_path.string() << "' ("
            << size << " bytes).\n";
}

int main(int argc, char *argv[]) {
  // <executable> <size> <filename> <directory location>
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <size_in_bytes> <filename> <directory_location>\n";
    return 1;
  }

  errno = 0;
  char *end = nullptr;
  long size = std::strtol(argv[1], &end, 10);
  if (errno != 0 || end == argv[1] || *end != '\0' || size < 0) {
    std::cerr << "Invalid size: " << argv[1] << "\n";
    return 1;
  }

  create_v_disk(size, argv[2], argv[3]);
  return 0;
}