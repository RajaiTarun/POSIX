#include "LSCommand.h"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <vector>
using namespace std;

void LSCommand::execute(const std::vector<std::string> tokens) {
  bool show_hidden = false;
  bool long_format = false;
  vector<string> target_dirs;

  for (int i = 1; i < tokens.size(); i++) {
    const string &token = tokens[i];
    if (token[0] == '-') {
      for (int j = 1; j < token.size(); j++) {
        if (token[j] == 'l') {
          long_format = true;
        } else if (token[j] == 'a') {
          show_hidden = true;
        } else {
          cerr << "Invalid option -- '" << token[j] << "'" << endl;
          return;
        }
      }
    } else {
      target_dirs.push_back(token);
    }
  }

  if (target_dirs.empty())
    target_dirs.push_back(".");

  for (int i = 0; i < target_dirs.size(); i++) {
    if (long_format) {
      print_long(target_dirs[i], show_hidden);
    } else {
      print_simple(target_dirs[i], show_hidden);
    }
  }
  if (target_dirs.size() > 1)
    cout << endl;
}

void LSCommand::print_simple(const std::string &path, bool show_hidden) {
  // we will open the dir at the path
  DIR *dir = opendir(path.c_str());
  // if dir == nullptr then there is no valid directory with this name
  if (dir == nullptr) {
    perror("ls");
    return;
  }

  // we have opened the directory, now let's get the filenames from the
  // directory
  vector<string> filenames;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    string name = entry->d_name;
    if (!show_hidden && name[0] == '.')
      continue;
    filenames.push_back(name);
  }
  closedir(dir);
  for (int i = 0; i < filenames.size(); i++) {
    cout << filenames[i] << endl;
  }
}

void LSCommand::print_long(const std::string &path, bool show_hidden) {
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    perror("ls");
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    string name = entry->d_name;
    if (!show_hidden && name[0] == '.')
      continue;

    // step 1 : i have to make full path, becaues struct stat needs full path
    // full path = path + / + entry -> d_name
    string full_path = path + "/" + name;

    // step 2 : now let's get the struct stat for this entry
    struct stat file_stat;
    if (stat(full_path.c_str(), &file_stat) < 0) {
      perror("stat");
      continue;
    }

    // step 3 : open passwd struct to get owner name
    struct passwd *pw = getpwuid(file_stat.st_uid);
    string owner_name;
    if (pw == nullptr) {
      owner_name = to_string(file_stat.st_uid);
      perror("pw");
    } else {
      owner_name = pw->pw_name;
    }

    // step 4 : now i want to get group id
    struct group *gr = getgrgid(file_stat.st_gid);
    string group_name;
    if (gr == nullptr) {
      group_name = to_string(file_stat.st_gid);
      perror("gr");
    } else {
      group_name = gr->gr_name;
    }

    // step 5 : now we want time
    char time_buf[64];
    // file_stat.st_mtime gives us the seconds that have passed after 1 january
    // 1970

    // to convert that in meaningful info we use struct tm, this gives us date
    // month hour minutes, etc
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    // to convert all that in string we do strftime
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);

    // step 6 : permissions
    string perms = "";
    perms += (S_ISDIR(file_stat.st_mode)) ? 'd' : '-';

    perms += (file_stat.st_mode & S_IRUSR) ? 'r' : '-';
    perms += (file_stat.st_mode & S_IWUSR) ? 'w' : '-';
    perms += (file_stat.st_mode & S_IXUSR) ? 'x' : '-';

    perms += (file_stat.st_mode & S_IRGRP) ? 'r' : '-';
    perms += (file_stat.st_mode & S_IWGRP) ? 'w' : '-';
    perms += (file_stat.st_mode & S_IXGRP) ? 'x' : '-';

    perms += (file_stat.st_mode & S_IROTH) ? 'r' : '-';
    perms += (file_stat.st_mode & S_IWOTH) ? 'w' : '-';
    perms += (file_stat.st_mode & S_IXOTH) ? 'x' : '-';

    cout << perms << " " << file_stat.st_size << "\t" << owner_name << "\t"
         << group_name << "\t" << time_buf << "\t" << name << endl;
  }
  closedir(dir);
}