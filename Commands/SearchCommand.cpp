#include "SearchCommand.h"
#include <dirent.h>
#include <iostream>
using namespace std;

bool SearchCommand::search_recursive(const std::string &base_dir,
                                     const std::string &target) {
  DIR *dir = opendir(base_dir.c_str());
  if (dir == nullptr) {
    return false;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    string name = entry->d_name;
    if (name == "." || name == "..")
      continue;

    if (name == target) {
      closedir(dir);
      return true;
    }

    if (entry->d_type == DT_DIR) {
      string full_path = base_dir + "/" + entry->d_name;
      bool ans = search_recursive(full_path, target);
      if (ans) {
        closedir(dir);
        return true;
      }
    }
  }
  closedir(dir);
  return false;
}

void SearchCommand::execute(const std::vector<std::string> &tokens) {
  if (tokens.size() < 2) {
    cout << "False" << endl;
    return;
  }

  bool found = search_recursive(".", tokens[1]);
  if (found) {
    cout << "True" << endl;
  } else {
    cout << "False" << endl;
  }
}