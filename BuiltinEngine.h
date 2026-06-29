#pragma once
#include "HistoryManager.h"
#include <iostream>
#include <string>
#include <vector>

class BuiltinEngine {
private:
  HistoryManager &historyManager;
  std::string homeDir;
  std::string prevDir;

  bool execute_pwd();
  bool execute_echo(const std::vector<std::string> &tokens);
  bool execute_cd(const std::vector<std::string> &tokens);
  bool execute_history(const std::vector<std::string> &tokens);

public:
  BuiltinEngine(std::string homeDir, HistoryManager &historyManager);
  bool execute(const std::vector<std::string> &tokens);
};