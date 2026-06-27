#include "BuiltinEngine.h"
#include "JobController.h"
#include "ProcessExecutor.h"

#include <csignal>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>

// #include <climits>     // gives the HOST_NAME_MAX constant
#include <cstdlib> // gives us getenv()
#include <pwd.h>   // gives us getpwuid() and the 'passwd' struct
#include <sstream>
#include <sys/types.h> // gives us the 'uid_t' type
#include <unistd.h>    // gives the gethostname(), getlogin() sys call
#include <vector>

using namespace std;

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

// helper function, i don't have c++ 20 so am creating starts_with function

// functions for building prompt
bool starts_with(const string &str, const string &prefix) {
  // just checks if the string str starts with prefix or not
  return str.find(prefix) == 0;
}

string getSystemName() {
  // this function returns computer's name
  // this is a blank c style character array
  char buffer[HOST_NAME_MAX];

  // this function asks the kernel to fill our buffer with host name and it
  // returns 0 for successful eexcution
  if (gethostname(buffer, HOST_NAME_MAX) == 0) {
    // we successfully got the host name, so we now conver the c style array to
    // c++ string and return it
    return string(buffer);
  } else {
    return "unkown host";
  }
}

string getUserName() {
  // this function returns current username of the system, for this we use the
  // function, getlogin() this returns char pointer string containg the
  // username, and returns nullptr if it fails. This function will fail if used
  // in an os where there is no login system, basically it is a stripped os
  // being used for tasks like : automated grading etc
  char *buffer;
  buffer = getlogin();
  if (buffer != nullptr)
    return string(buffer);

  // fallback if the above methods fails, robust and will work on stripped os,
  // automated grader type usecases also
  struct passwd *pw = getpwuid(geteuid());
  if (pw != nullptr && pw->pw_name != nullptr) {
    return string(pw->pw_name);
  }

  return "anonymous";
}

string getHomeDirectory() {
  // when a terminal opens, the OS injects an environment variable called
  // "HOME",in our process
  const char *homeDirectory = getenv("HOME");
  if (homeDirectory != nullptr)
    return string(homeDirectory);

  // if the above method returns nullptr, we switch to more robus way
  // we basically get the systems id and then get the passwd structure for that
  // and it hase the home directory in pw_dir
  homeDirectory = getpwuid(geteuid())->pw_dir;
  if (homeDirectory == nullptr)
    return "/";
  return string(homeDirectory);
}

string getCurrentWorkingDirectory() {
  // this function returns the current working directory, we use the getcwd()
  // function for this, it fills the buffer and if the function fails it returns
  // nullptr, else returns a valid pointer to our buffer
  char buffer[PATH_MAX];
  if (getcwd(buffer, PATH_MAX) == nullptr) {
    return "unknown_dir";
  }
  return string(buffer);
}

string contractHomeDirectory(const string &cwd, const string &home) {
  if (home == cwd)
    return "~";

  if (starts_with(cwd, home)) {
    if (cwd.length() == home.length() || cwd[home.length()] == '/') {
      string contractedHomeDirectory = '~' + cwd.substr(home.length());
      return contractedHomeDirectory;
    }
  }

  return cwd;
}

// functions for building lexer
bool isPureWhitespace(const string &str) {
  int i = 0;
  while (i < str.length() && isspace(str[i])) {
    i++;
  }
  return (i == str.length());
}

vector<string> splitCommands(const string &rawInput) {
  // we first get the raw input from the user, and split it when we find ';'
  // so inorder to split and push in the vector we first convert the rawInput to
  // a stringstream
  vector<string> commands;
  stringstream ss(rawInput);
  string chunk;

  while (getline(ss, chunk, ';')) {
    if (chunk.empty() || isPureWhitespace(chunk))
      continue;
    commands.push_back(chunk);
  }

  return commands;
}

vector<CommandStage> parsePipeline(const vector<string> &rawTokens) {
  vector<CommandStage> stages;
  CommandStage currentStage;

  for (size_t i = 0; i < rawTokens.size(); i++) {
    const string &token = rawTokens[i];

    if (token == "|") {
      // Pipe hit! Push the completed box and reset for the next command.
      stages.push_back(currentStage);
      currentStage = CommandStage();
    } else if (token == "<") {
      // basically after this char we have the input file for our input command,
      // so there should be a token after this index
      if (i + 1 < rawTokens.size()) {
        currentStage.inputFile = rawTokens[i + 1];
        i++;
      }
      // else means that there is no input file, and this is an error
      else {
        cerr << "Syntax error : no input file specified" << endl;
      }
    } else if (token == ">" || token == ">>") {
      if (token == ">>") {
        currentStage.isAppend = true;
      } else {
        currentStage.isAppend = false;
      }
      if (i + 1 < rawTokens.size()) {
        currentStage.outputFile = rawTokens[i + 1];
        i++;
      } else {
        cerr << "Syntax error : not output file specified" << endl;
      }
    } else {
      // Regular command argument (e.g., "ls", "-la", "/tmp")
      currentStage.tokens.push_back(token);
    }
  }

  // Push the final stage after the loop finishes
  if (!currentStage.tokens.empty() || !currentStage.inputFile.empty() ||
      !currentStage.outputFile.empty()) {
    stages.push_back(currentStage);
  }

  return stages;
}

vector<string> tokenize(const string &rawChunk) {
  vector<string> tokenized;
  stringstream ss(rawChunk);
  string chunk;
  while (ss >> chunk) {
    tokenized.push_back(chunk);
  }
  return tokenized;
}

// The hardware-safe signal flag
volatile sig_atomic_t sigint_pressed = 0;

void sigint_handler(int sig) {
  sigint_pressed = 1; // Flip the flag when Ctrl+C is pressed
}

int main() {
  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  // signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  string hostName = getSystemName();
  string userName = getUserName();
  string homeDir = getHomeDirectory();

  BuiltinEngine builtin(homeDir);
  JobController jobController;
  ProcessExecutor processExecutor;

  while (1) {
    // we first check if any background process is done or not
    jobController.sweepCompletedJobs();

    string cwd = getCurrentWorkingDirectory();
    string displayPath = contractHomeDirectory(cwd, homeDir);

    // displaying the prompt
    string prompt =
        "<" + userName + "@" + hostName + ":" + displayPath + "> : ";
    cout << prompt << flush;

    // taking input from user
    string rawInput;
    if (!getline(cin, rawInput)) {

      if (sigint_pressed) {
        sigint_pressed = 0;
        cin.clear();
        clearerr(stdin);
        cout << "\n";
        continue;
      }
      // case A : The user pressed Ctrl + D(EOF) we must exit gracefully
      if (cin.eof()) {
        cout << "\n";
        break;
      }
    }

    // now we need to make chunk based on ';'
    vector<string> commandChunks = splitCommands(rawInput);

    // now we need to preprocess this and make tokens from the chunks
    vector<string> tokens;
    for (int i = 0; i < commandChunks.size(); i++) {
      string rawChunk = commandChunks[i];
      if (rawChunk.find('|') != string::npos ||
          rawChunk.find('<') != string::npos ||
          rawChunk.find('>') != string::npos) {
        vector<string> flatTokens = tokenize(rawChunk);
        if (flatTokens.empty())
          continue;

        vector<CommandStage> stages = parsePipeline(flatTokens);

        processExecutor.execute_pipeline(stages);
        continue;
      }
      tokens.clear();
      tokens = tokenize(commandChunks[i]);
      if (tokens.empty())
        continue;
      if (tokens[0] == "exit") {
        return 0;
      }

      bool isBackground = false;
      if (tokens.back() == "&") {
        isBackground = true;
        tokens.pop_back();
      }

      string cleanCmdText = "";
      for (int j = 0; j < tokens.size(); j++) {
        cleanCmdText += tokens[j];
        if (j != tokens.size() - 1)
          cleanCmdText += " ";
      }
      // checking if the command is builtin
      bool isBuiltin = builtin.execute(tokens);

      // if it is not a builtin, giving it to processExecutor
      if (!isBuiltin) {
        if (isBackground) {
          processExecutor.execute_background(tokens, cleanCmdText,
                                             jobController);
        } else {
          processExecutor.execute_foreground(tokens);
        }
      }
    }
  }
  return 0;
}