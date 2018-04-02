#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <dirent.h>
#include <cstring>

#define CHUNK 1024
#define CAT "cat"
#define CD "cd"
#define CHMOD "chmod"
#define ECHO "echo"
#define EXIT "exit"
#define FIND "find"
#define HELP "help"
#define ID "id"
#define MKDIR "mkdir"
#define PWD "pwd"
#define RM "rm"
#define RMDIR "rmdir"
#define STAT "stat"
#define TOUCH "touch"
#define UMASK "umask"

void parse_command();
std::vector<std::string> tokenize(std::string str);
std::string parse_mode(mode_t);

void cat_command();   // OK
void cd_command();    // OK
void chmod_command(); // OK
void echo_command();  // OK
void exit_command();  // OK
void find_command();  // OK
void help_command();  // OK
void id_command();    // OK
void mkdir_command(); // OK
void pwd_command();   // OK
void rm_command();    // OK
void rmdir_command(); // OK
void stat_command();  // OK
void touch_command(); // OK
void umask_command(); // OK

std::string help_msg =
    "cat {file}:              Display content of {file}.\n"
    "cd {dir}:                Switch current working directory to {dir}.\n"
    "chmod {mode} {file/dir}: Change the mode (permission) of a file or directory.\n"
    "                         {mode} is an octal number.\n"
    "echo {str} [filename]:   Display {str}. If [filename] is given,\n"
    "                         open [filename] and append {str} to the file.\n"
    "exit:                    Leave the shell.\n"
    "find [dir]:              List files/dirs in the current working directory\n"
    "                         or [dir] if it is given.\n"
    "                         Minimum outputs contain file type, size, and name.\n"
    "help:                    Display help message.\n"
    "id:                      Show current euid and egid.\n"
    "mkdir {dir}:             Create a new directory {dir}.\n"
    "pwd:                     Print the current working directory.\n"
    "rm {file}:               Remove a file.\n"
    "rmdir {dir}:             Remove an empty directory.\n"
    "stat {file/dir}:         Display detailed information of the given file/dir.\n"
    "touch {file}:            Create {file} if it does not exist,\n"
    "                         or update its access and modification timestamp.\n"
    "umask {mode}:            Change the umask of the current session.\n";
std::vector<std::string> tokens;

std::string parse_mode(mode_t mode)
{
    std::string str = "---------- ";
    if (S_ISDIR(mode)) // test if the file is directory or not.
        str[0] = 'd';

    if (mode & S_IRUSR)
        str[1] = 'r'; /* 3 bits for user  */
    if (mode & S_IWUSR)
        str[2] = 'w';
    if (mode & S_IXUSR)
        str[3] = 'x';
    if (mode & S_ISUID)
        str[3] = 's';

    if (mode & S_IRGRP)
        str[4] = 'r'; /* 3 bits for group */
    if (mode & S_IWGRP)
        str[5] = 'w';
    if (mode & S_IXGRP)
        str[6] = 'x';
    if (mode & S_ISGID)
        str[6] = 's';

    if (mode & S_IROTH)
        str[7] = 'r'; /* 3 bits for other */
    if (mode & S_IWOTH)
        str[8] = 'w';
    if (mode & S_IXOTH)
        str[9] = 'x';
    if (mode & S_ISVTX)
    {
        str[9] = 't';
        str[10] = '.';
    }

    return str.c_str();
}

void cat_command()
{
    FILE *fd;
    size_t content;
    char buf[CHUNK];
    int len;

    fd = fopen(tokens[1].c_str(), "r");

    if (fd != NULL)
    {
        fseek(fd, 0, SEEK_END);
        len = ftell(fd);
        rewind(fd);
        while ((content = fread(buf, 1, sizeof buf, fd)) > 0)
            fwrite(buf, 1, content, stdout);

        fclose(fd);
    }
    else
    {
        perror("File does not exist.");
    }
}

void cd_command()
{
    int state;

#ifdef DEBUG
    std::cout << "DEBUG MODE!!!!" << std::endl;
#endif
    state = chdir(tokens[1].c_str());

    if (state != 0)
    {
        perror("Error occur when changing directory.");
    }
}

void chmod_command()
{
    int state;
    mode_t mode;
    mode = strtoul(tokens[1].c_str(), NULL, 8);
    state = chmod(tokens[2].c_str(), mode);
    if (state != 0)
        perror("Error occur when changing file/directory modes.");
}

void echo_command()
{
    FILE *fd;
    if (tokens.size() == 2)
    {
        std::cout << tokens[1].c_str() << std::endl;
    }
    else if (tokens.size() == 3)
    {
        fd = fopen(tokens[2].c_str(), "a");
        if (fd != NULL)
        {
            fwrite(tokens[1].append("\n").c_str(), sizeof(char), strlen(tokens[1].c_str()) + 1, fd);
            fclose(fd);
        }
        else
        {
            perror("Error occur when opening the file.");
        }
    }
}

void exit_command()
{
    std::cout << "Bye~" << std::endl;
    exit(0);
}

void find_command()
{
    DIR *dir;
    struct dirent *ent;
    char buf[300];
    size_t size = 300;

    if (tokens.size() == 1)
    {
        // list files / dirs in the current working directory
        getcwd(buf, size);
        dir = opendir(buf);
    }
    else
    {
        // or [dir] if it is given.
        dir = opendir(tokens[1].c_str());
    }

    if (dir != NULL)
    {
        // Minimum outputs contain file type, size, and name.\n
        while ((ent = readdir(dir)) != NULL)
        {
            struct stat stat_buf;
            int state;
            std::string full_path;
            if (tokens.size() == 1) {
            	std::string str_buf(buf);
            	str_buf = buf;
            	full_path = str_buf + "/" + ent->d_name;
            } else {
            	full_path = tokens[1] + "/" + ent->d_name;
            }
            state = stat(full_path.c_str(), &stat_buf);
            if (state != 0)
                perror("Error");
            std::cout << parse_mode(stat_buf.st_mode) << "\t";
            std::cout << stat_buf.st_size << "\t";
            std::cout << ent->d_name << std::endl;
        }
        closedir(dir);
    }
    else
    {
        perror("Could not open the directory.");
    }
}

void help_command()
{
    std::cout << help_msg << std::endl;
}

void id_command()
{
    std::cout << "euid: " << geteuid() << ", egid: " << getegid() << std::endl;
}

void mkdir_command()
{
    int state;
    if (tokens.size() > 2)
        perror("Passing in too many arguments.");
    else if (tokens.size() == 1)
        perror("Please pass in the name of directory.");

    state = mkdir(tokens[1].c_str(), 0755);
    if (state != 0)
        perror("Error occur when creating the directory.");
}

void pwd_command()
{
    char buf[300];
    size_t size = 300;
    getcwd(buf, size);
    std::cout << buf << std::endl;
}

void rm_command()
{
    int state;
    state = remove(tokens[1].c_str());
    if (state != 0)
        perror("Error occur when removing file.");
}

void rmdir_command()
{
    int state;
    state = rmdir(tokens[1].c_str());
    if (state != 0)
        perror("rmdir_command rmdir() error");
}

void stat_command()
{
    int state;
    struct stat buf;
    state = stat(tokens[1].c_str(), &buf);
    if (state != 0)
    {
        perror("stat_command stat() error");
    }
    else
    {
        std::cout << "File: \'" << tokens[1].c_str() << "\'" << std::endl;
        std::cout << "Size: " << buf.st_size << "\t";
        std::cout << "Blocks: " << buf.st_blocks << "\t";
        std::cout << "IO Block: " << buf.st_blksize << std::endl;

        std::cout << "Device: " << buf.st_dev << "d"
                  << "\t";
        std::cout << "Inode: " << buf.st_ino << "\t";
        std::cout << "Links: " << buf.st_nlink << std::endl;

        std::cout << "Access: " << parse_mode(buf.st_mode) << "\t";
        std::cout << "Uid: " << buf.st_uid << "\t";
        std::cout << "Gid: " << buf.st_gid << std::endl;

        struct tm access_ts, modify_ts, change_ts;
        char access_buf[80], modify_buf[80], change_buf[80];
        access_ts = *localtime(&buf.st_atim.tv_sec);
        modify_ts = *localtime(&buf.st_mtim.tv_sec);
        change_ts = *localtime(&buf.st_ctim.tv_sec);
        strftime(access_buf, sizeof(access_buf), "%Y-%m-%d %H:%M:%S %Z", &access_ts);
        strftime(modify_buf, sizeof(modify_buf), "%Y-%m-%d %H:%M:%S %Z", &modify_ts);
        strftime(change_buf, sizeof(change_buf), "%Y-%m-%d %H:%M:%S %Z", &change_ts);

        std::cout << "Access time: " << access_buf << std::endl;
        std::cout << "Modify time: " << modify_buf << std::endl;
        std::cout << "Change time: " << change_buf << std::endl;
        std::cout << "Birth: -" << std::endl;
    }
}

void touch_command()
{
    int state_creat, state_utime; // 664

    if (access(tokens[1].c_str(), F_OK) != -1)
    {
        // file exists, update file access and modification timestamp.
        state_utime = utime(tokens[1].c_str(), NULL);
        if (state_utime != 0)
            perror("Error");
    }
    else
    {
        // file doesn't exist
        state_creat = creat(tokens[1].c_str(), 0644);
        if (state_creat == -1)
            perror("Error");
    }
}

void umask_command()
{
    mode_t cmask;
    cmask = strtoul(tokens[1].c_str(), NULL, 8);
    umask(cmask);
}

std::vector<std::string> tokenize(std::string str)
{
    std::vector<std::string> tokens;
    std::istringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ' '))
        tokens.push_back(token);

    return tokens;
}

int main(int argc, char *argv[])
{
    size_t bufsize = 32;
    size_t chars;

    while (1)
    {
        std::string str;
        printf("$ ");
        std::getline(std::cin, str);
        if (str[str.length() - 2] == '\r') {
            str[str.length() - 2] = '\n';
            str = str.substr(0, str.length() - 1);
        }
        if (str[str.length() - 1] == '\n')
            str = str.substr(0, str.length() - 1);
        tokens = tokenize(str);
        parse_command();
    }
}

void parse_command()
{
    if (CAT == tokens[0])
        cat_command();
    else if (CD == tokens[0])
        cd_command();
    else if (CHMOD == tokens[0])
        chmod_command();
    else if (ECHO == tokens[0])
        echo_command();
    else if (EXIT == tokens[0])
        exit_command();
    else if (FIND == tokens[0])
        find_command();
    else if (HELP == tokens[0])
        help_command();
    else if (ID == tokens[0])
        id_command();
    else if (MKDIR == tokens[0])
        mkdir_command();
    else if (PWD == tokens[0])
        pwd_command();
    else if (RM == tokens[0])
        rm_command();
    else if (RMDIR == tokens[0])
        rmdir_command();
    else if (STAT == tokens[0])
        stat_command();
    else if (TOUCH == tokens[0])
        touch_command();
    else if (UMASK == tokens[0])
        umask_command();
    else
        std::cout << "Unknown command." << std::endl;
}
