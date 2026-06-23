#pragma once
#include <string>
#include <vector>

class ProcessExecutor {
private:
  std::vector<char *> convertToc_args(const std::vector<std::string> &tokens);

public:
  bool execute_foreground(const std::vector<std::string> &tokens);
};