#include "shell_functions.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::vector<std::string> parse_line(std::string line, bool& background) {
    // returns each word in its own position
    std::vector<std::string> args_list;
    std::string word = "";
    for (decltype(word.size()) i = 0; i < line.size(); ++i) {
        const char c = line.at(i);
        // ignore newline chars
        if (c == '\n') {
            continue;
        }
        if (c == '<' || c == '>' || c == '|') {
            if (word != "")
                args_list.push_back(word);
            args_list.push_back(std::string(1, c));
            word = "";
            continue;
        }
        // word should be empty at this point
        if (c == '"' || c == '\'') {
            ++i;
            for (; line.at(i) != c; ++i) {
                word += line.at(i);
            }
            args_list.push_back(word);
            word = "";
            continue;
        }

        if (c == '$') {
            std::string temp = "";
            for (; line.at(i) != ')'; ++i) {
                temp += line.at(i);
            }
            temp += line.at(i);
            word += dollar_expansion(temp);
            continue;
        }

        if (c == ' ') {
            if (word != "")
                args_list.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }

    if (word.size() > 0)
        args_list.push_back(word);

    if (args_list.at(args_list.size() - 1) == "&") {
        background = true;
        args_list.pop_back();
    } else {
        background = false;
    }

    return args_list;
}

int redirect(std::vector<std::string>& args_list, std::string mode) {
    // checks for > and redirects and returns file descriptor for closing
    // removes > and file name from args_list
    // if no >, returns -1 and doesnt change args_list
    int fd = -1;

    for (decltype(args_list.size()) i = 0; i < args_list.size(); ++i) {
        if (mode == "output" && args_list.at(i) == ">") {
            fd = open(args_list[i + 1].c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
            if (fd < 0) {
                std::cerr << "Error creating file " << args_list[i + 1] << std::endl;
                return -1;
            }
            // removes > and file name from std::vector
            args_list.erase(args_list.begin() + i, args_list.begin() + i + 2);
            // redirects stdout to new file
            dup2(fd, 1);
            return fd;
        } else if (mode == "input" && args_list.at(i) == "<") {
            fd = open(args_list[i + 1].c_str(), O_RDONLY);
            if (fd < 0) {
                std::cerr << "Error opening file " << args_list[i + 1] << std::endl;
                return -1;
            }
            // removes < and file name from std::vector
            args_list.erase(args_list.begin() + i, args_list.begin() + i + 2);
            // redirects stdin from file
            dup2(fd, 0);
            return fd;
        }
    }
    return fd;
}

bool no_pipes(char* args[]) {
    for (int i = 0; args[i] != NULL; ++i) {
        if (!strcmp(args[i], "|"))
            return false;
    }
    return true;
}

void execute_pipes(std::vector<std::string>& args_list) {
    char* buf[args_list.size() + 1];
    int cnt = 0;
    for (int i = 0; i < args_list.size(); ++i) {
        int p[2];
        if (pipe(p) == -1) {
            std::cerr << "Error creating pipe" << std::endl;
        }

        if (args_list[i] != "|") {
            buf[cnt] = (char*)args_list[i].c_str();
            ++cnt;
            if (i == args_list.size() - 1) { // last iteration
                buf[cnt] = NULL;
                execvp(buf[0], buf);
            }
        } else {

            int pid = fork();
            if (pid == 0) { // child process
                buf[cnt] = NULL;
                if (dup2(p[1], 1) == -1) {
                    std::cerr << "ERROR redirecting stdout to pipe" << std::endl;
                }
                execvp(buf[0], buf);
            }
            // parent process
            if (dup2(p[0], 0) == -1) {
                std::cerr << "ERROR redirecting stdin to pipe" << std::endl;
            }
            close(p[1]);
            waitpid(pid, 0, 0);
            cnt = 0;
        }
    }
}

void check_background_ps(std::vector<int>& bg_ps) {
    std::vector<int> tmp(bg_ps.size());
    for (int pid : bg_ps) {
        int status;
        waitpid(pid, &status, WNOHANG);
        if (status == 0) { // background process still running
            tmp.push_back(pid);
        } else if (status < 0) { // background process error
            std::cerr << "Background process ended with error" << std::endl;
        }
        // otherwise it the process was reaped successfully
    }
    // move tmp into bg_ps
    bg_ps = move(tmp);
}

bool change_dir(std::vector<std::string>& parsed, std::string& prev_dir) {
    if (parsed.at(0) == "cd") {
        char cwd[BUF_SIZE];
        getcwd(cwd, sizeof(cwd));
        std::string cwd_str(cwd);
        std::string username = getenv("USER");
        std::string home_dir = "/home/" + username;
        const char* home =  home_dir.c_str();

        if (parsed.size() == 1 || parsed.at(1) == "~") {
            chdir(home);
        } else if (parsed.at(1) == "-") {
            if (prev_dir != "") {
                chdir(prev_dir.c_str());
            }
        } else {
            chdir(parsed.at(1).c_str());
        }
        prev_dir = cwd_str;
        return true;
    }
    return false;
}

void execute(std::vector<std::string>& parsed) {

    int fd_out = redirect(parsed, "output");
    int fd_in  = redirect(parsed, "input");

    char* args[parsed.size() + 1];
    for (decltype(parsed.size()) i = 0; i < parsed.size(); ++i) {
        args[i] = (char*)parsed[i].c_str();
    }
    args[parsed.size()] = NULL;

    if (no_pipes(args)) {
        // dup2(1, 0);
        execvp(args[0], args);
    } else {
        execute_pipes(parsed);
    }
}

std::string dollar_expansion(std::string dol) {
    // remove $ and ()
    dol.erase(dol.begin(), dol.begin() + 2);
    dol.erase(dol.end() - 1, dol.end());

    int p[2];
    if (pipe(p) == -1) {
        std::cerr << "Error creating pipe" << std::endl;
    }

    if (fork() == 0) { // child process
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);

        bool trash;
        std::vector<std::string> parsed = parse_line(dol, trash);
        execute(parsed);

    } else { // parent process
        close(p[1]);
        // dup2(p[0], 0);

        char buf[BUF_SIZE];
        read(p[0], buf, BUF_SIZE);
        wait(NULL);
        std::string new_expr(buf);
        new_expr.pop_back(); // remove std::endline char
        return new_expr;
    }
    return "ERROR";
}