#include <iostream>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <algorithm>

#define PS "ps"
#define EXIT "exit"

uid_t uid;
gid_t gid;
int col_index = 0;

std::vector<std::string> get_pid_dir();
std::vector<std::vector<std::string> > read_stat(std::vector<std::string> pid_dir);
bool is_number(const std::string& s);
void parse_cmd(std::vector<std::string>);
std::vector<std::string> tokenize(std::string);
void print_ps(std::vector<std::vector<std::string> > rows, bool show_all);

bool is_number(const std::string &s) {
    // to determine if a string consists of all numbers.
    // find will return npos if the value is not found.
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

bool sort_col(const std::vector<std::string> &item1, const std::vector<std::string> &item2) {
    int int_item1, int_item2;
    std::istringstream iss1(item1[col_index]);
    std::istringstream iss2(item2[col_index]);
    iss1 >> int_item1;
    iss2 >> int_item2;

    return int_item1 < int_item2;
}


std::vector<std::string> tokenize(std::string s) {
    // separate string by spaces
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    while (iss) {
        std::string subs;
        iss >> subs;
        tokens.push_back(subs);
    }
    return tokens;
}


std::vector<std::string> get_pid_dir() {
    // get the pid dir in /proc
    DIR *dir;
    struct dirent *ent;
    // char buf[300];
    std::vector<std::string> pid_dir;

    if ((dir = opendir("/proc")) != nullptr)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr)
        {
            // only store the dir which name is number (pid)
            if (is_number(ent->d_name)) {
                std::string d_name(ent->d_name);
                std::string full_path = std::string("/proc/") + d_name;
                pid_dir.push_back(full_path);
            }
        }
        closedir(dir);
    } else
    {
        /* could not open directory */
        perror("Error occur when opening /proc");
    }

    return pid_dir;
}

std::vector<std::vector<std::string> > read_stat(std::vector<std::string> pid_dir) {
    // read the stat in pid directory.
    // pid, uid, gid, ppid, pgid, sid, tty, St, (img), cmd
    std::vector<std::string> row;
    std::vector<std::vector<std::string> > rows;
    std::vector<std::string>::iterator it;

    for (it = pid_dir.begin(); it != pid_dir.end(); ++it) {
        std::ifstream f_stat(*it + std::string("/stat"));
        std::ifstream f_status(*it + std::string("/status"));

        // separate txt in stat file using white space.
        std::string stat, status, uid, gid;
        if (f_stat.is_open() && f_status.is_open()) {
            std::getline(f_stat, stat);

            for (size_t i = 0; i < 7; ++i) {
                std::getline(f_status, status);
            }
            std::getline(f_status, uid);
            std::getline(f_status, gid);
        } else {
            perror("Cannot open file.");
        }
        std::vector<std::string> token_stat = tokenize(stat);
        std::vector<std::string> token_uid = tokenize(uid);
        std::vector<std::string> token_gid = tokenize(gid);

        row.push_back(token_stat[0]);
        row.push_back(token_uid[1]);
        row.push_back(token_gid[1]);
        for (size_t i = 3; i < 7; ++i) {
            // ppid, pgid, sid, tty
            row.push_back(token_stat[i]);
        }
        row.push_back(token_stat[2]);  // state
        row.push_back(token_stat[1]);  // img
        // row.push_back(token_stat[3]);  // cmd??
        rows.push_back(row);
        row.clear();
        // break;
    }

    return rows;
}



void parse_cmd(std::vector<std::string> tokens) {
    // print the stat according to the commands.
    if (EXIT == tokens[0]) {
        std::cout << "Bye~" << std::endl;
        exit(0);
    }
    if (PS != tokens[0]) {
        std::cout << "Unknow command." << std::endl;
    } else {
        /*
        -a: can be used to list processes from all the users.
        -x: can be used to list processes without an associated terminal.
        */
        std::vector<std::string> pid_dir = get_pid_dir();
        std::vector<std::vector<std::string> >  rows = read_stat(pid_dir);
        bool print_all = false;
        col_index = 0;

        if ("" == tokens[1]) {
            // default
            col_index = 0;
            print_all = false;
        }
        if ("-a" == tokens[1]) {
            print_all = true;
        }
        if ("-x" == tokens[1]) {
            
        }
        // -p, -q, -r, and -s, which sort the listed processes by pid (default), ppid, pgid, and sid, respectively.
        if ("-p" == tokens[1]) {
            col_index = 0;
        } else if ("-q" == tokens[1]) {
            col_index = 3;
        } else if ("-r" == tokens[1]) {
            col_index = 4;
        } else if ("s" == tokens[1]) {
            col_index = 5;
        }

        if (col_index != 0) {
            std::sort(rows.begin(), rows.end(), sort_col);
        }

        print_ps(rows, print_all);
    }
}

void print_ps(std::vector<std::vector<std::string> > rows, bool show_all) {
    std::vector<std::vector<std::string> >::iterator its;
    std::vector<std::string>::iterator it;

    std::cout << "pid\tuid\tgid\tppid\tpgid\tsid\ttty\tSt\t(img)\tcmd" << std::endl;

    if(show_all) {
        for (its = rows.begin(); its != rows.end(); ++its) {
            for (it = its->begin(); it != its->end(); ++it) {
                std::cout << (*it) << "\t";
            }
            std::cout << std::endl;
        }
    } else {
        for (its = rows.begin(); its != rows.end(); ++its) {
            // show only the process that the user owns
            std::vector<std::string> row = (*its);
            if ((row[1] == std::to_string(uid)) && 
                (row[2] == std::to_string(gid))) {
                for (it = its->begin(); it != its->end(); ++it)
                    std::cout << (*it) << "\t";
                std::cout << std::endl;
            } 
        }
    }
}

int main(int argc, char *argv[]) {
    setlinebuf(stdin);
    setvbuf(stdout, NULL, _IONBF, 0);

    // get euid & egid of the user
    uid = getuid();
    gid = getgid();

    while(1) {
        std::string str;
        std::cout << "$ ";
        std::getline(std::cin, str);
        if ((str[str.length() - 1] == '\r') ||
            (str[str.length() - 1] == '\n')) {
            str = str.substr(0, str.length() - 1);
        } 
        std::vector<std::string> tokens = tokenize(str);
        parse_cmd(tokens);
    }
    return 0;
}