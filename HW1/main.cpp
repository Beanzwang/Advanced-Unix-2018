#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <dirent.h>

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
void check_error();

void cat_command(); // OK
void cd_command();  // OK
void chmod_command();
void echo_command(); // OK
void exit_command(); // OK
void find_command(); // OK
void help_command();
void id_command(); // OK
void mkdir_command(); // OK
void pwd_command();   // OK
void rm_command();    // OK
void rmdir_command(); // OK
void stat_command();
void touch_command(); // OK
void umask_command();

std::string help_msg = "Help";
std::vector<std::string> tokens;

void cat_command() {
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

void cd_command() {
    int state;
    std::cout << "Change to " << tokens[1] << std::endl;
    state = chdir(tokens[1].c_str());

    if (state != 0)
    {
        perror("Error occur when changing directory.");
    }
}

void chmod_command() {
    // TODO: handle mode_t: tokens[1]
    int state;
    state = chmod(tokens[2].c_str(), 777);
    if (state != 0)
        perror("Error occur when changing file/directory modes.");
}

void echo_command() {
    FILE *fd;
    if (tokens.size() == 2) {
        std::cout << tokens[1].c_str() << std::endl;
    }
    else if (tokens.size() == 3)
    {
        fd = fopen(tokens[2].c_str(), "a");
        if (fd != NULL)
        {
            fwrite(tokens[1].append("\n").c_str(), sizeof(char), strlen(tokens[1].c_str())+1, fd);
            fclose(fd);
        }
        else
        {
            perror("Error occur when opening the file.");
        }
    }
}

void exit_command() {
    std::cout << "Bye~" << std::endl;
    exit(0);
}

void find_command() {
    DIR *dir;
    struct dirent *ent;
    char buf[300];
    size_t size = 300;
    getcwd(buf, size);

    if ((dir = opendir(buf)) != NULL)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL)
        {
            printf("%s\n", ent->d_name);
        }
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        perror("Could not open the directory.");
    }
}

void help_command() {
    std::cout << help_msg << std::endl;
}

void id_command() {
    std::cout << "euid: " << geteuid() << ", egid: " << getegid() << std::endl;
}

void mkdir_command() {
    int state;
    if (tokens.size() > 2)
        perror("Passing in too many arguments.");
    else if (tokens.size() == 1)
        perror("Please pass in the name of directory.");

    state = mkdir(tokens[1].c_str(), 0755);
    if (state != 0)
        perror("Error occur when creating the directory.");
}

void pwd_command() {
    char buf[300];
    size_t size = 300;
    getcwd(buf, size);
    std::cout << buf << std::endl;
}

void rm_command() {
    int state;
    state = remove(tokens[1].c_str());
    if (state != 0)
        perror("Error occur when removing file.");
}

void rmdir_command() {
    int state;
    state = rmdir(tokens[1].c_str());
    if (state != 0)
        perror("Error occur when removing the directory.");
}

void stat_command() {
    int state;
    struct stat *buf;
    state = stat(tokens[1].c_str(), buf);
    if (state != 0) {
        perror("Error occur when printing stat.");
    }
}

void touch_command() {

}

void umask_command() {
    mode_t cmask;
    // sscanf(argv[1], "%ld", cmask);
    umask(cmask);
}

void check_error() {

}

std::vector<std::string> tokenize(std::string str) {
    std::vector<std::string> tokens;
    std::istringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ' '))
        tokens.push_back(token);
    
    return tokens;
}

int main(int argc, char* argv[]) {
    size_t bufsize = 32;
    size_t chars;
    
    while (1) {
        std::string str;
        printf("$ ");
        std::getline(std::cin, str);
        if (str[str.length() - 1] == '\n')
            str = str.substr(0, str.length() - 1);
        tokens = tokenize(str);
        parse_command();
    }
}

void parse_command() {
    if (strcmp(CAT, tokens[0].c_str()) == 0)
        cat_command();
    else if (strcmp(CD, tokens[0].c_str()) == 0)
        cd_command();
    else if (strcmp(CHMOD, tokens[0].c_str()) == 0)
        chmod_command();
    else if (strcmp(ECHO, tokens[0].c_str()) == 0)
        echo_command();
    else if (strcmp(EXIT, tokens[0].c_str()) == 0)
        exit_command();
    else if (strcmp(FIND, tokens[0].c_str()) == 0)
        find_command();
    else if (strcmp(HELP, tokens[0].c_str()) == 0)
        help_command();
    else if (strcmp(ID, tokens[0].c_str()) == 0)
        id_command();
    else if (strcmp(MKDIR, tokens[0].c_str()) == 0)
        mkdir_command();
    else if (strcmp(PWD, tokens[0].c_str()) == 0)
        pwd_command();
    else if (strcmp(RM, tokens[0].c_str()) == 0)
        rm_command();
    else if (strcmp(RMDIR, tokens[0].c_str()) == 0)
        rmdir_command();
    else if (strcmp(STAT, tokens[0].c_str()) == 0)
        stat_command();
    else if (strcmp(TOUCH, tokens[0].c_str()) == 0)
        touch_command();
    else if (strcmp(UMASK, tokens[0].c_str()) == 0)
        umask_command();
    else
        std::cout << "Unknown command." << std::endl;
}