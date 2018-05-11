#include <iostream>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <algorithm>
#include <iomanip>

uid_t uid;
gid_t gid;

struct Row {    
    pid_t pid;
    uid_t uid;
    gid_t gid;
    pid_t ppid;
    gid_t pgid;
    pid_t sid;
    pid_t tty;
    char St;
    char img[100];
    char cmd[200];
};

std::vector<std::string> get_pid_dir();
std::vector<Row> read_stat(std::vector<std::string> pid_dir);
bool is_number(const std::string &s);
void print_ps(std::vector<Row> rows, bool show_all);
void print_indt(Row row);
bool sort_ppid(const Row &row1, const Row &row2);
bool sort_pgid(const Row &row1, const Row &row2);
bool sort_sid(const Row &row1, const Row &row2);

bool is_number(const std::string &s) {
    // to determine if a string consists of all numbers.
    // find will return npos if the value is not found.
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

bool sort_ppid(const Row &row1, const Row &row2) {
    return row1.ppid < row2.ppid;
}

bool sort_pgid(const Row &row1, const Row &row2) {
    return row1.pgid < row2.pgid;
}

bool sort_sid(const Row &row1, const Row &row2) {
    return row1.sid < row2.sid;
}

std::vector<std::string> get_pid_dir() {
    // get the pid dir in /proc
    DIR *dir;
    struct dirent *ent;
    std::vector<std::string> pid_dir;

    if ((dir = opendir("/proc")) != nullptr)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr) {
            // only store the dir which name is number (pid)
            if (is_number(ent->d_name)) {
                std::string d_name(ent->d_name);
                std::string full_path = std::string("/proc/") + d_name;
                pid_dir.push_back(full_path);
            }
        }
        closedir(dir);
    } else {
        /* could not open directory */
        perror("Error occur when opening /proc");
    }

    return pid_dir;
}

std::vector<Row> read_stat(std::vector<std::string> pid_dir) {
    // read the stat in pid directory.
    // pid, uid, gid, ppid, pgid, sid, tty, St, (img), cmd
    // std::vector<std::string> row;
    std::vector<Row> rows;
    std::vector<std::string>::iterator it;

    for (it = pid_dir.begin(); it != pid_dir.end(); ++it) {
        FILE *f_stat = fopen((*it + std::string("/stat")).c_str(), "r");
        FILE *f_status = fopen((*it + std::string("/status")).c_str(), "r");
        FILE *f_cmd = fopen((*it + std::string("/cmdline")).c_str(), "r");

        // separate txt in stat file using white space.
        // pid, uid, gid, ppid, pgid, sid, tty, St, (img), cmd
        Row row;
        char stat[200], status[200], name[200];
        
        if (f_stat == NULL || f_status == NULL || f_cmd == NULL)
            perror("Error opening file.");
        
        std::fgets(stat, 200, f_stat);
        sscanf(stat, "%d %s %c %d %d %d %d", &row.pid, row.img, &row.St, &row.ppid, &row.pgid, &row.sid, &row.tty);

        std::fgets(row.cmd, 200, f_cmd);

        for (size_t i = 0; i < 7; ++i) {
            std::fgets(status, 200, f_status);
        }
        std::fgets(status, 200, f_status);
        sscanf(status, "%s %d", name, &row.uid);
        std::fgets(status, 200, f_status);
        sscanf(status, "%s %d", name, &row.gid);

        rows.push_back(row);

        fclose(f_stat);
        fclose(f_status);
        fclose(f_cmd);
    }

    return rows;
}


void print_ps(std::vector<Row> rows, bool show_all) {
    std::vector<Row>::iterator it;

    std::cout << std::setw(5) << std::right << "pid" << std::setw(5) << "uid" << std::setw(5) << "gid";
    std::cout << std::setw(6) << std::right << "ppid" << std::setw(6) << "pgid" << std::setw(6) << "sid";
    std::cout << std::setw(8) << std::right << "tty";
    std::cout << " St (img) cmd" << std::endl;

    if (show_all) {
        for (it = rows.begin(); it != rows.end(); ++it) {
            print_indt(*it);
        }
    } else {
        for (it = rows.begin(); it != rows.end(); ++it) {
            // show only the process that the user owns
            Row row = (*it);
            if ((row.uid == uid) && (row.gid == gid)) {
                print_indt(row);
            }
        }
    }
}

void print_indt(Row row) {
    std::cout << std::setw(5) << std::right << row.pid << std::setw(5) << row.uid << std::setw(5) << row.gid;
    std::cout << std::setw(6) << std::right << row.ppid << std::setw(6) << row.pgid << std::setw(6) << row.sid;
    std::cout << std::setw(8) << std::right << row.tty << std::setw(3) << row.St;
    std::cout << " " << row.img << " " << row.cmd << std::endl;
}

int main(int argc, char *argv[]) {
    // get euid & egid of the user
    uid = getuid();
    gid = getgid();

    // print the stat according to the commands.
    if (argc > 4 || argc < 2) {
        std::cout << "Error: too few or too many arguments." << std::endl;
        return 0;
    }

    if (strcmp("ps", argv[1]) != 0) {
        std::cout << "Error: Unknow command." << std::endl;
        return 0;
    }
    
    bool print_all = false;  //  "-a"
    bool sort_q = false;  // sort ppid
    bool sort_r = false;  // sort pgid
    bool sort_s = false;  // sort sid

    std::vector<std::string> pid_dir = get_pid_dir();
    std::vector<Row> rows = read_stat(pid_dir);

    bool valid_cmd = true;
    for (int i = 2; i < argc; ++i) {
        valid_cmd = false;
        /*
        -a: can be used to list processes from all the users.
        -x: can be used to list processes without an associated terminal.
        -p, -q, -r, and -s, which sort the listed processes by pid (default), ppid, pgid, and sid, respectively.
        */
        if (strcmp("-a", argv[i]) == 0) {
            valid_cmd = true;
            print_all = true;
        }
        // if (strcmp("-x", argv[i]) == 0) print_all = true;
        if (strcmp("-p", argv[i]) == 0) {
            valid_cmd = true;
        }
        if (strcmp("-q", argv[i]) == 0) {
            std::sort(rows.begin(), rows.end(), sort_ppid);
            valid_cmd = true;
            sort_q = true;
        }
        if (strcmp("-r", argv[i]) == 0) {
            std::sort(rows.begin(), rows.end(), sort_pgid);
            valid_cmd = true;
            sort_r = true;
        }
        if (strcmp("-s", argv[i]) == 0) {
            std::sort(rows.begin(), rows.end(), sort_sid);
            valid_cmd = true;
            sort_s = true;
        }
        if (!valid_cmd) {
            std::cout << "Error: Unknown command." << std::endl;
            return 0;
        }
    }

    if (sort_q && sort_r && sort_s) std::cout << "Error: You can only sort one element at a time." << std::endl;

    print_ps(rows, print_all);
    
    return 0;
}