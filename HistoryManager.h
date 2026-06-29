#pragma once
#include <deque>
#include <string>
#include <vector>
class HistoryManager {
private:
  std::deque<std::string> historyLedger;
  std::string historyFilePath;

public:
  HistoryManager(const std::string &homeDir);
  void loadFromFile();
  void addCommand(const std::string &inputCommand);
  void saveToFile();
  std::deque<std::string> getHistory(int numOfCommands = 10);
};