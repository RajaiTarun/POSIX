#include "BuiltinEngine.h"
#include "Commands/LSCommand.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
using namespace std;

BuiltinEngine::BuiltinEngine(string homeDir) {
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
  }
  return false;
}