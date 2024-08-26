#ifndef SHELL_FUNCTIONS_H
#define SHELL_FUNCTIONS_H
#include <string>
#include <vector>

inline const int BUF_SIZE = 256;

std::vector<std::string> parse_line(std::string line, bool& background);
int redirect(std::vector<std::string>& args_list, std::string mode);
bool no_pipes(char* args[]);
void execute_pipes(std::vector<std::string>& args_list);
void check_background_ps(std::vector<int>& bg_ps);
bool change_dir(std::vector<std::string>& parsed, std::string& prev_dir);
void execute(std::vector<std::string>& parsed);
std::string dollar_expansion(std::string dol);

#endif