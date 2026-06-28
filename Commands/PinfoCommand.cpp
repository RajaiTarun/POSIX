#include "PinfoCommand.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
using namespace std;

void PinfoCommand::execute(const std::vector<std::string> tokens) {
  // we want to print
  // process id
  // process status
  // memory
  // executable path

  // let's first get the target process id
  //   if user types pinfo pid then target id = pid, and if user only types
  //   pinfo then target pid = our current shell process id
  pid_t target_pid;
  if (tokens.size() > 1) {
    // validating the target pid given
    string input_pid = tokens[1];
    bool is_number = true;
    if (input_pid.empty())
      is_number = false;
    for (char c : input_pid) {
      if (c < '0' || c > '9')
        is_number = false;
      if (!is_number)
        break;
    }

    if (!is_number) {
      cerr << "pinfo : invalid pid" << endl;
      return;
    }

    target_pid = stoi(input_pid);
  } else {
    // getting the current shells process id
    target_pid = getpid();
  }

  // basically the linux kernel maintains a single line text file(for all active
  // processes on ram) having the info about the processes that are currently on
  // and it also maintains executable path info in /proc/pid/exe
  // the ram, path for that info is : /proc/process_id/stat
  string stat_path = "/proc/" + to_string(target_pid) + "/stat";

  // so we open that file
  ifstream stat_file;
  stat_file.open(stat_path);

  if (!stat_file.is_open()) {
    cerr << "pinfo : Process : " << target_pid << " does not exist" << endl;
    return;
  }

  // whatever is in that file will now be transferred to line
  string line;
  getline(stat_file, line);
  istringstream iss(
      line); // we converted that in stringstream to get words from that line

  vector<string> stats;
  string word;
  // now we push all the words in vector stats
  // and it will have total 52 entries, but we will only use 2, 4, 7 and 22
  // entry
  while (iss >> word) {
    stats.push_back(word);
  }

  //   stats[2] is status of the process
  //   stats[4] is the process group id of the pid
  //   stats[7] is the terminal process group id, this is the process group that
  //   is active stats[22] is the memory taken by the process in ram
  // so one trick that we do is : if(stats[2] == stats[7]) this means that out
  // process is the active process currently, so we add a '+' to the status
  string status = stats[2];
  string pgrp_id = stats[4];
  string tpgrp_id = stats[7];
  if (pgrp_id == tpgrp_id)
    status += '+';
  string memory = stats[22];

  // now let's get the executable path
  // the content at the the /proc/pid/exe will be a link, so we need to read
  // that link and that can't be done using ifstream, if we do it using ifstream
  // that it will give random garbage characters, so we need to use readlink for
  // that
  string exe_path = "/proc/" + to_string(target_pid) + "/exe";

  char buffer[1024];
  string exec_path_str;
  ssize_t len = readlink(exe_path.c_str(), buffer, sizeof(buffer) - 1);

  if (len != -1) {
    buffer[len] = '\0';
    exec_path_str = buffer;
  } else {
    exec_path_str = "Executable path unreadable";
  }

  cout << "pid -- " << target_pid << endl;
  cout << "Process Status -- " << status << endl;
  cout << "memory -- " << memory << " {Virtual Memory}" << endl;
  cout << "Executable Path -- " << exec_path_str << endl;
}