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
  // boolean flags for long format and hidden files
  bool show_hidden = false;
  bool long_format = false;

  // a ls command can have more than one directory, so will store them in a
  // vector if no directory present then we work on the present folder and that
  // is : '.'
  vector<string> target_dirs;

  // the -l, -a, -la, -al, etc flags can be present anywhere in the command
  // tokens will look like : ["ls", "-l / -a / -la / -al", "dir1", "dir2"]
  for (int i = 1; i < tokens.size(); i++) {
    const string &token = tokens[i];
    // if the tokens first char is '-' means that this will be a flag
    if (token[0] == '-') {
      for (int j = 1; j < token.size(); j++) {
        if (token[j] == 'l') {
          long_format = true;
        } else if (token[j] == 'a') {
          show_hidden = true;
        }
        // there are only two valid flags
        else {
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

  // now that we have all the target directories and the long format and hidden
  // files state, we can start printing
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
  // in this we only need to print the file names
  // we will open the dir at the path
  DIR *dir = opendir(path.c_str());
  // if dir == nullptr then there is no valid directory with this name
  if (dir == nullptr) {
    perror("ls");
    return;
  }

  // we have opened the directory, now let's get the filenames from the
  // to get the file entries of a partircular directory : dir
  //   we do struct dirent* entry and when we do entry = readdir(dir) and we
  //   keep doing this till we get a nullptr in entry and entry -> d_name gives
  //   the name of the file
  // directory
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    string name = entry->d_name;
    if (!show_hidden && name[0] == '.')
      continue;
    cout << name << endl;
  }
  closedir(dir);
}

void LSCommand::print_long(const std::string &path, bool show_hidden) {
  // in long format we need to print : permissions -> size -> owner name ->
  // group name -> last mod time -> filename

  // steps to get all these info :
  // 1. filename : same as print simple, we do entry = readdir(dir) and get name
  // from entry -> d_name
  // 2. we create a full path, and then open struct stat for that file,
  // file_stat
  // 3. get uid from struct stat using file_stat.st_uid
  // 4. we pass this uid in getpwuid(file_stat.st_uid) and get a passwd struct
  // and store that in struct passwd* pw = getpwuid(file_stat.st_uid)
  // 5. now from the pw struct we can get owner name, pw -> pw_name
  // 6. now to get group name, we first get the group id from file_stat struct
  // using file_stat.st_gid then we similarlly create struct group *gr =
  // getgrgid(file_stat.st_gid), now we can get group name from gr -> gr_name
  // 7. now to get time what we do is : we first do file_stat.st_mtime and this
  // gives us seconds that have passed till now from jan 1 1970 and now to
  // convert this to meaningful time we have tm struct in c++ so we make a
  // struct struct tm* tm_info = localtime(&file_stat.st_mtime). Now to get date
  // month hour minutes we have a function, that is : strftime(time_buf,
  // sizeof(time_buf), "%b %d %H:%M", tm_info) here time_buf is the char array
  // where we store the answer and %b is month in short, %d is date H and M are
  // hours and minutes
  // 8. now to get permissions we do bitwise and of file_stat.st_mode with the
  // required fields and S_ISDIR to check if it is directory or not

  // same process as print_simple
  // opening the directory
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    perror("ls");
    return;
  }

  // getting entries from the directory
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    // getting file name
    string name = entry->d_name;
    if (!show_hidden && name[0] == '.')
      continue;

    // step 1 : i have to make full path, becaues struct stat needs full path
    // full path = path + / + entry -> d_name
    string full_path = path + "/" + name;

    // step 2 : now let's get the struct stat for this entry
    struct stat file_stat;
    int links = file_stat.st_nlink;
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

    cout << perms << "\t" << links << "\t" << file_stat.st_size << "\t"
         << owner_name << "\t" << group_name << "\t" << time_buf << "\t" << name
         << endl;
  }
  closedir(dir);
}