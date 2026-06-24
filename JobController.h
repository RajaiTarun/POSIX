#pragma once
#include <string>
#include <sys/types.h>
#include <vector>

struct BackgroundJob {
  int jobId;
  pid_t pid;
  std::string command;
};

class JobController {
  int nextJobId;
  std::vector<BackgroundJob> activeJobs;

public:
  JobController();
  void addJob(pid_t pid, const std::string &command);
  void sweepCompletedJobs();
};