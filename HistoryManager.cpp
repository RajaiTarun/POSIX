#include "HistoryManager.h"
#include <cstdio>
#include <deque>
#include <fstream>
#include <string>
using namespace std;

// i am naming the history file as : .historyState.txt

HistoryManager::HistoryManager(const string &homeDir) {
  historyFilePath = homeDir + "/.historyState.txt";
  loadFromFile();
}

void HistoryManager::loadFromFile() {
  ifstream historyFile;
  historyFile.open(historyFilePath);

  if (!historyFile.is_open()) {
    // this means there might be no file so let's initilaize a empty dequeu
    historyLedger = deque<string>();
  } else {
    // we need to laod the old history in the deque
    string line;
    while (getline(historyFile, line)) {
      if (!line.empty()) {
        historyLedger.push_back(line);
        if (historyLedger.size() > 20)
          historyLedger.pop_front();
      }
    }
  }
  historyFile.close();
}

void HistoryManager::addCommand(const std::string &inputCommand) {
  if (inputCommand.empty() ||
      (!historyLedger.empty() && historyLedger.back() == inputCommand))
    return;
  historyLedger.push_back(inputCommand);
  if (historyLedger.size() > 20) {
    historyLedger.pop_front();
  }
}

void HistoryManager::saveToFile() {
  ofstream historyFile(historyFilePath);
  if (!historyFile.is_open()) {
    perror("ofstream open");
  } else {
    for (auto it = historyLedger.begin(); it != historyLedger.end(); it++) {
      historyFile << *it << "\n";
    }
  }
  historyFile.close();
}

deque<string> HistoryManager::getHistory(int numOfCommands) {
  int numCommandsInLedger = historyLedger.size();
  if (numOfCommands > 20)
    numOfCommands = 20;
  if (numOfCommands > numCommandsInLedger)
    numOfCommands = numCommandsInLedger;

  deque<string> historyResult;
  int count = 1;
  for (auto it = historyLedger.rbegin();
       it != historyLedger.rend() && count <= numOfCommands; it++) {
    historyResult.push_back(*it);
    count++;
  }
  return historyResult;
}
