#pragma once
#include <string>
#include <vector>
class LSCommand {
private:
  void print_simple(const std::string &path, bool show_hidden);
  void print_long(const std::string &path, bool show_hidden);

public:
  void execute(const std::vector<std::string> tokens);
};