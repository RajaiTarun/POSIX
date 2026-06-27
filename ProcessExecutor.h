#pragma once
#include "JobController.h"
#include <string>
#include <vector>

struct CommandStage {
  std::vector<std::string> tokens;
  std::string inputFile = "";
  std::string outputFile = "";
  bool isAppend = false;
};

class ProcessExecutor {
private:
  std::vector<char *> convertToc_args(const std::vector<std::string> &tokens);

public:
  bool execute_foreground(const std::vector<std::string> &tokens);

  bool execute_background(const std::vector<std::string> &tokens,
                          const std::string &rawCommandString,
                          JobController &JobController);
  bool execute_pipeline(const std::vector<CommandStage> &stages);
};