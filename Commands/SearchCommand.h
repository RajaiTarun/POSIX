#pragma once
#include <string>
#include <vector>

class SearchCommand {
private:
  bool search_recursive(const std::string &base_dir, const std::string &target);

public:
  void execute(const std::vector<std::string> &tokens);
};