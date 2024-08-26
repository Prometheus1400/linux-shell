// #include <curses.h>
#include "shell_functions.h"
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
using namespace std;

int main() {

    time_t _tm = time(NULL);
    vector<int> bg_ps;
    string prev_dir = "";
    while (true) {
        string curtime = static_cast<string>(asctime(localtime(&_tm)));
        curtime.pop_back();
        check_background_ps(bg_ps); // checking backgrounded processes
        cout << "prom: " << curtime << "$ ";
        cout.flush();

        string inputline;
        getline(cin, inputline);

        if (inputline == string("exit")) {
            cout << "Bye!! End of shell" << endl;
            break;
        }

        // preparing the input command for execution
        // parsing input line
        bool background;
        vector<string> parsed = parse_line(inputline, background);
        // for (auto w : parsed) {
        //     cout << w << endl;
        // }

        if (change_dir(parsed, prev_dir))
            continue;

        int pid = fork();
        if (pid == 0) { // child process
            execute(parsed);
        } else { // parent
            if (background) {
                bg_ps.push_back(pid);
            } else {
                // wait for the child process
                // we will discuss why waitpid() is preferred over wait()
                waitpid(pid, 0, 0);
            }
        }
    }
}
