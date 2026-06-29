#include "BuiltinEngine.h"
#include "Commands/LSCommand.h"
#include "Commands/PinfoCommand.h"
#include "Commands/SearchCommand.h"
#include "HistoryManager.h"
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
using namespace std;

BuiltinEngine::BuiltinEngine(string homeDir, HistoryManager &historyManager)
    : historyManager(historyManager) {
  prevDir = "";
  this->homeDir = homeDir;
}

bool BuiltinEngine::execute_pwd() {
  char buffer[1024];
  if (getcwd(buffer, sizeof(buffer)) != nullptr) {
    cout << buffer << endl;
  } else {
    perror("pwd error");
  }
  return true;
}

bool BuiltinEngine::execute_echo(const std::vector<std::string> &tokens) {
  for (int i = 1; i < tokens.size(); i++) {
    cout << tokens[i];
    if (i != tokens.size() - 1)
      cout << " ";
  }
  cout << endl;
  return true;
}

bool BuiltinEngine::execute_cd(const std::vector<std::string> &tokens) {
  char currDirBuffer[1024];
  if (getcwd(currDirBuffer, sizeof(currDirBuffer)) == nullptr) {
    perror("getcwd failed in execute_cd");
    return true;
  }

  if (tokens.size() > 2) {
    cerr << "Invalid arguments" << endl;
    return true;
  }

  string target;

  if (tokens.size() == 1) {
    // this means user just typed "cd" and we need to cd to the home directory
    target = this->homeDir;
  } else {
    target = tokens[1];
    if (target == "-") {
      if (this->prevDir.length() == 0) {
        cerr << "prevDir not set" << endl;
        return true;
      }
      target = this->prevDir;
      cout << target << endl;
    } else if (target.find('~') == 0) {
      target = this->homeDir + target.substr(1);
    }
  }

  if (chdir(target.c_str()) == 0) {
    this->prevDir = string(currDirBuffer);
  } else {
    perror("cd");
  }
  return true;
}

bool BuiltinEngine::execute_history(const std::vector<std::string> &tokens) {
  int numToShow = 10;
  if (tokens.size() > 2) {
    cerr << "invalid history command arguments";
    return true;
  }
  if (tokens.size() > 1) {
    for (int i = 0; i < tokens[1].size(); i++) {
      if (tokens[1][i] < '0' || tokens[1][i] > '9') {
        cerr << "invalid history command arguments" << endl;
        return true;
      }
    }
    try {
      numToShow = stoi(tokens[1]);
    } catch (...) {
      cerr << "invalid history command arguments" << endl;
      return true;
    }
  }

  deque<string> ledger = historyManager.getHistory(numToShow);
  for (auto it = ledger.rbegin(); it != ledger.rend(); it++) {
    cout << *it << endl;
  }
  return true;
}

bool BuiltinEngine::execute(const std::vector<std::string> &tokens) {
  if (tokens[0] == "pwd") {
    return execute_pwd();
  } else if (tokens[0] == "echo") {
    return execute_echo(tokens);
  } else if (tokens[0] == "cd") {
    return execute_cd(tokens);
  } else if (tokens[0] == "ls") {
    LSCommand LS;
    LS.execute(tokens);
    return true;
  } else if (tokens[0] == "search") {
    SearchCommand S;
    S.execute(tokens);
    return true;
  } else if (tokens[0] == "pinfo") {
    PinfoCommand PI;
    PI.execute(tokens);
    return true;
  } else if (tokens[0] == "history") {
    return execute_history(tokens);
  }
  return false;
}