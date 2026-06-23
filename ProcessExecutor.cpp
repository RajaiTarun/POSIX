#include "ProcessExecutor.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <sys/types.h> // Gives us 'pid_t'
#include <sys/wait.h>  // Gives us waitpid() and its macros
#include <unistd.h>    // Gives us fork() and execvp()
using namespace std;

vector<char *> ProcessExecutor::convertToc_args(const vector<string> &tokens) {
  vector<char *> c_args;
  // 1. Reserving memory in advance to make the loop insanely fast and we add +1
  // for the mandatory nullptr at the end
  c_args.reserve(tokens.size() + 1);

  // 2. now converting the tokens to char* ancient c style strings
  for (int i = 0; i < tokens.size(); i++) {
    c_args.push_back(const_cast<char *>(tokens[i].c_str()));
  }

  // 3. adding the mandatory nullptr
  c_args.push_back(nullptr);
  return c_args;
}

bool ProcessExecutor::execute_foreground(const vector<string> &tokens) {
  // checking if tokens.empty(). If yes, return false.
  if (tokens.empty())
    return false;

  // converting tokens into a std::vector<char*> c_args ending explicitly with
  // nullptr.
  vector<char *> c_args = convertToc_args(tokens);

  // creating a child process
  pid_t pid = fork();

  if (pid == 0) {
    // this is the child process, because pid is 0 in child process and non zero
    // in parent process

    // now in this child process we will call the execvp function
    execvp(c_args[0], c_args.data());

    // now if the CPU reaches this lines, means execvp failed and so we return a
    // perror
    perror(c_args[0]);
    // and so we kill the child process
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    return true;
  } else {
    // this means the fork returned -1 and this is an OS failure, so we raise
    // and perror
    perror("fork failed");
    return false;
  }
}
