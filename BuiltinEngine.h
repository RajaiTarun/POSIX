#pragma once
#include <iostream>
#include <string>
#include <vector>

class BuiltinEngine {
private:
  std::string homeDir;
  std::string prevDir;

  bool execute_pwd();
  bool execute_echo(const std::vector<std::string> &tokens);
  bool execute_cd(const std::vector<std::string> &tokens);

public:
  BuiltinEngine(std::string homeDir);
  bool execute(const std::vector<std::string> &tokens);
};