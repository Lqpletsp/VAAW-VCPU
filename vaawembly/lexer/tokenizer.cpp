#include "tokenizer.hpp"
#include <fstream>
#include <unordered_map>
#include <vector>

enum class token_types {
  constant,
  identifier,
  instruction,
  set,
  jmp,  // jump
  jrmp, // relative jump
  call,
  ret,
  preg0,
  preg1,
  preg2,
  preg3,
  preg4,
  preg5,
  preg6,
  preg7,
  preg8,
  invalid,
};

struct token_md { // token_md -> token metadata
  int line, column;
  token_types main_type;
};

static std::vector<std::string> main_code;
static std::vector<std::vector<std::string>> tokenized_code;

static file_extract_t extract_text_from_file(std::string &code_file) {
  std::ifstream InFile(code_file);
  if (!InFile.is_open())
    return {.error_message = "unable to open the intended file",
            .status = false};
  std::string line;
  while (std::getline(InFile, line)) {
    main_code.push_back(line);
  }
  return {.error_message = "", .status = true};
}
static void handle_code_lines(std::string &line) {
  std::unordered_map<std::string, token_types> str_inst_map = {
      {"set", token_types::set},     {"jmp", token_types::jmp},
      {"jrmp", token_types::jrmp},   {"call", token_types::call},
      {"ret", token_types::ret},     {"preg0", token_types::preg0},
      {"preg1", token_types::preg1}, {"preg2", token_types::preg2},
      {"preg3", token_types::preg3}, {"preg4", token_types::preg4},
      {"preg5", token_types::preg5}, {"preg6", token_types::preg6},
      {"preg7", token_types::preg7}, {"preg8", token_types::preg8}};

  auto determine_token_type = [&](std::string str) {
    if (str.size() == 0)
      return token_types::invalid;
    auto str_inst_map_data = str_inst_map.find(str);
    if (str_inst_map_data == str_inst_map.end()) {
      // constant
      try {
        std::stoi(str);
        return token_types::constant;
      } catch (...) {
      }
      return token_types::identifier;
    }
    return str_inst_map_data->second;
  };
  std::string token;
  for (size_t l_i = 0; l_i < line.size(); ++l_i) {
    char ch = line.at(l_i);
    if (ch == ' ') {
    }
  }
}
static void tokenize_code() {
  for (size_t mc_i = 0; mc_i < main_code.size(); ++mc_i) {
  }
}
