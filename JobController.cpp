#include "JobController.h"
#include <cstddef>
#include <iostream>
#include <sys/wait.h>
using namespace std;

JobController::JobController() { nextJobId = 1; }

void JobController::addJob(pid_t pid, const std::string &command) {
  BackgroundJob temp;
  temp.pid = pid;
  temp.command = command;
  temp.jobId = nextJobId++;
  JobController::activeJobs.push_back(temp);
  cout << "[ " << temp.jobId << " ] " << pid << endl;
}

void JobController::sweepCompletedJobs() {
  int status;
  pid_t reapedPid;

  while ((reapedPid = waitpid(-1, &status, WNOHANG)) > 0) {
    for (size_t i = 0; i < activeJobs.size(); i++) {
      if (activeJobs[i].pid == reapedPid) {
        cout << "[ " << activeJobs[i].jobId << " ] done "
             << activeJobs[i].command << endl;
        activeJobs.erase(activeJobs.begin() + i);
        break;
      }
    }
  }
}