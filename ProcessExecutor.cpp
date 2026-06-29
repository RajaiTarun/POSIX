#include "ProcessExecutor.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
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
  for (size_t i = 0; i < tokens.size(); i++) {
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
    // FOREGROUND CHILD: Take off the vest so the user can kill me!
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
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
    waitpid(pid, &status, WUNTRACED);
    if (WIFSTOPPED(status)) {
      cout << "\n[Suspended] PID: " << pid << endl;
    }
    return true;
  } else {
    // this means the fork returned -1 and this is an OS failure, so we raise
    // and perror
    perror("fork failed");
    return false;
  }
}

bool ProcessExecutor::execute_background(const std::vector<std::string> &tokens,
                                         const std::string &rawCommandString,
                                         JobController &JobController) {
  if (tokens.empty()) {
    return false;
  }

  vector<char *> c_args = convertToc_args(tokens);

  pid_t pid = fork();
  if (pid == 0) {
    execvp(c_args[0], c_args.data());
    perror(c_args[0]);
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    JobController.addJob(pid, rawCommandString);
    return true;
  } else {
    perror("fork failed");
    return false;
  }
}

bool ProcessExecutor::execute_pipeline(const vector<CommandStage> &stages) {
  int prev_read_fd = -1;
  vector<pid_t> child_pids;

  for (size_t i = 0; i < stages.size(); i++) {
    int pipefd[2] = {-1, -1};
    bool notLastStage = (i < stages.size() - 1);

    if (notLastStage) {
      if (pipe(pipefd) < 0) {
        perror("pipe creation failed");
        return false;
      }
    }

    pid_t pid = fork();

    if (pid == 0) {
      // =============================================================
      //                  CHILD PROCESS (Electrician at work)
      // =============================================================

      // --- 1. INPUT WIRING (Plug 0) ---
      if (!stages[i].inputFile.empty()) {
        int in_fd = open(stages[i].inputFile.c_str(), O_RDONLY);
        if (in_fd < 0) {
          perror("unable to open input file");
          exit(EXIT_FAILURE);
        }
        dup2(in_fd, STDIN_FILENO);
        close(
            in_fd); // we can close in_fd, becuase we have cloned that in slot 0
      } else if (prev_read_fd != -1) {
        dup2(prev_read_fd, STDIN_FILENO);
        close(prev_read_fd);
      }

      // --- 2. OUTPUT WIRING (Plug 1) ---
      if (!stages[i].outputFile.empty()) {
        int flags;

        if (stages[i].isAppend) {
          flags = O_WRONLY | O_CREAT | O_APPEND;
        } else {
          flags = O_WRONLY | O_CREAT | O_TRUNC;
        }

        int out_fd = open(stages[i].outputFile.c_str(), flags, 0644);
        if (out_fd < 0) {
          perror("unable to open output file");
          exit(EXIT_FAILURE);
        }
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
      } else if (notLastStage) {
        // Agle command ko pipe ke zariye bhej do
        dup2(pipefd[1], STDOUT_FILENO);
      }

      // --- 3. CHILD SAFETY CLEANUP ---
      // Child ke andar jo extra pipes khule reh gaye hain unhe band karo taaki
      // hang na ho!
      if (notLastStage) {
        close(pipefd[0]);
        close(pipefd[1]);
      }

      // --- 4. EXECUTE COMMAND ---
      vector<char *> c_args = convertToc_args(stages[i].tokens);
      execvp(c_args[0], c_args.data());

      // Agar execvp fail ho jaye (jaise galat command type kiya ho)
      perror(c_args[0]);
      exit(EXIT_FAILURE);
    } else if (pid > 0) {
      // =============================================================
      //                  PARENT PROCESS (Supervisor)
      // =============================================================
      child_pids.push_back(pid);

      // Pichla pipe ab kisi kaam ka nahi
      if (prev_read_fd != -1) {
        close(prev_read_fd);
      }

      // Naye pipe ka Drain parent ko nahi chahiye
      if (notLastStage) {
        close(pipefd[1]);
        prev_read_fd = pipefd[0]; // Agle cycle ke liye Tap yaad rakho
      }
    } else {
      perror("fork failed");
      return false;
    }
  }

  // --- 5. THE UNDERTAKER (Teeno bachhon ke marne ka wait karo) ---
  for (size_t i = 0; i < child_pids.size(); i++) {
    int status;
    waitpid(child_pids[i], &status, 0);
  }

  return true;
}
