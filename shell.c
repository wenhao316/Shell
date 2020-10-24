/**
 * Shell
 * CS 241 - Spring 2020
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include "sstring.h"
#include <assert.h>


typedef struct process {
    char *command;
    pid_t pid;
    time_t time;
} process;

// GLOBAL VARIABLES
static pid_t curr_child = -1;
static vector* process_queue;
static int MAX_BUFFER_SIZE = 1000;
static int MAX_CHAR_NUM = 20;
// static int BASE_NUM = 10;
static int TIME_BASE = 6;

// Process
    //Allocate space
process* allocate_process(pid_t pid, const char* buffer) {
    process* tmp = (process*)malloc(sizeof(process));
    tmp -> pid = pid;
    tmp -> command = (char*)calloc(strlen(buffer) + 1, sizeof(char));
    strcpy(tmp -> command, buffer);
    return tmp;
}

    //Free process queue
void free_process(pid_t pid) {
    size_t i;
    for (i = 0; i < vector_size(process_queue); i++) {
        process* temp = vector_get(process_queue, i);
        if (temp -> pid == pid) {
            free(temp -> command);
            free(temp);
            vector_erase(process_queue, i);
            return;
        }
    }
}
    //Skip space and create

// INTERACTION
    //Write history
    void write_history(FILE* file, char* input, vector* h) {
        vector_push_back(h, input);
        if (file) {
            fprintf(file, "%s\n",input);
            fflush(file);
        }
    }

    //Read history
    vector* read_history(FILE* file) {
        // vector* buffer = string_vector_create();
        size_t len = 0;
        char* line = NULL;
        vector* h = string_vector_create();
        if (file) {
            while (getline(&line, &len, file) >= 0) {
                line[strlen(line - 1)] = '\0';
                if (strlen(line)) 
                    vector_push_back(h, line);
                
                free(line);
                line = NULL;
            } 
            free(line);
        }
        return h;
    }

    //Crtl C pressed
    void ctrlc_signal() {
        if (curr_child != getpgid(curr_child)) {
            kill(curr_child, SIGINT);
        }
    }

    //Background
    void background_signal() {
        int status = 0;
        while (waitpid(-1, &status, WNOHANG) > 0) {}
    }


// COMMAND

    //Get last command
char* pop_command(vector* input) {
    size_t s = vector_size(input) - 1;
    return (char*)vector_get(input, s);
}
    //Get last line
char pop_line(char* input) {
    char temp = input[strlen(input) - 1];
    return temp;
}

    //Built in command
bool check_built_in_command(vector* input) {
    size_t i;
    for (i = 0; i < vector_size(input); i++) {
        if (strcmp(vector_get(input, i), "cd") == 0) {
            return true;
        }
        if (strcmp(vector_get(input, i), "!history") == 0) {
            return true;
        }
        if (strcmp(vector_get(input, i), "#") == 0) {
            return true;
        }

        if (strcmp(vector_get(input, i), "!") == 0) {
            return true;
        }
    }
    return false;
}

    //Get specific string from vector
        // if n = 0, return last char string
char* get_char_string(vector* input, size_t n) {
    size_t l = vector_size(input);
    if (n == 0) {
        return (char*)vector_get(input, l - 1);
    }
    return (char*)vector_get(input, n);
}

    //Logical Operator
bool is_logical_operator(vector* input) {
    size_t i;
    for (i = 0; i < vector_size(input); i++) {
        if (strcmp(vector_get(input, i), "&&") == 0) {
            return true;
        }
        if (strcmp(vector_get(input, i), "||") == 0) {
            return true;
        }
        if (((char*)vector_get(input, i))[strlen(vector_get(input, i)) - 1] == ';') {
            return true;
        }
    }
    return false;
}

    // a for &&, o for ||, s for ;
char get_logical_operator(vector* input, size_t n) {
        if (strcmp(vector_get(input, n), "&&") == 0) {
            return 'a';
        }
        if (strcmp(vector_get(input, n), "||") == 0) {
            return 'o';
        }
        if (((char*)vector_get(input, n))[strlen(vector_get(input, n)) - 1] == ';') {
            return 's';
        }
    return 'n';
}

    //Fork Exec Wait command
int f_e_w_command(vector* input) {  
    size_t v_size = vector_size(input);
    char* end = pop_command(input);
    bool flag = false;
    if (strcmp(end, "&") == 0) {
        vector_pop_back(input);
        v_size--;
        flag = true;
    } 
    else if (end[strlen(end) - 1] == '&') {
        end[strlen(end) - 1] = '\0';
        flag = true;
    }
    
    char* command_cpy[v_size + 1];

    for (size_t i = 0; i < v_size; i++) {
        command_cpy[i] = vector_get(input, i);
    }

    command_cpy[v_size] = NULL;
    process* tmp_p = malloc(sizeof(process));
    tmp_p->time = time(NULL);

    if (vector_empty(input)) {
        tmp_p -> command = NULL;
    } else {
    sstring* c_sstring = cstr_to_sstring(" ");
    sstring* s = cstr_to_sstring("");
    for (size_t i = 0; i < vector_size(input) - 1; i++) {
        sstring *tmp = cstr_to_sstring(vector_get(input, i));
        sstring_append(s, tmp);
        sstring_append(s, c_sstring);
        sstring_destroy(tmp);
    }
    sstring *end = cstr_to_sstring(vector_get(input, vector_size(input)-1));
    sstring_append(s, end);
    sstring_destroy(end);

    sstring_destroy(c_sstring);
    char *res = sstring_to_cstr(s);
    sstring_destroy(s);
    tmp_p -> command = res;
    }
        
    vector_push_back(process_queue, tmp_p);

    pid_t p = fork();
    curr_child = p;
    if (p < 0) {
        print_fork_failed();
        exit(1);
    } 
    
    else if (p == 0) {
        execvp(command_cpy[0], command_cpy);
        print_exec_failed(command_cpy[0]);
        exit(1);
    } else {
        print_command_executed(p);
        tmp_p -> pid = p;
        if (flag) {
            if (setpgid(p, p) == -1) {
                print_setpgid_failed();
                exit(1);
            }
        } else {
            int status = 0;
            if (waitpid(p, &status, 0) == -1) {
                print_wait_failed();
                exit(1);
            }
            if (WIFEXITED(status) && WEXITSTATUS(status)) {
                free_process(p);
                return 1;
            }
            free_process(p);
        }
    }

    return 0;
}

char get_seperator(vector* input, size_t n) {
    return ((char *)vector_get(input, n))[strlen(vector_get(input, n)) - 1];
}

    //Fork Exec Wait line
int f_e_w_line(vector* input) {
    vector* tmp = string_vector_create();

    for (size_t i = 0; i < vector_size(input); i++) {

        // AND
        if (strcmp(vector_get(input, i), "&&") == 0) {
            int res = f_e_w_command(tmp);
            if (res != 0) {
                vector_destroy(tmp);
                return res;
            } else {
                vector_clear(tmp);
            }
        } 
        
        // OR
        else if (strcmp(vector_get(input, i), "||") == 0) {
            if (f_e_w_command(tmp) != 0) {
                vector_clear(tmp);
            } else {
                vector_destroy(tmp);
                return 0;
            }
        } 
        
        // SEPERATOR
        else if (get_seperator(input, i) == ';') {
            char* end = (char*)vector_get(input, i);
            end[strlen(end) - 1] = '\0';
            if (strlen(end))
                vector_push_back(tmp, end);
            end[strlen(end)] = ';';
            f_e_w_command(tmp);
            vector_clear(tmp);
            continue;
        } else {
            vector_push_back(tmp, vector_get(input, i));
        }
    }
    int exec_c = f_e_w_command(tmp);
    vector_destroy(tmp);
    return exec_c;
}

vector* filter_space(char* command) {
    char* tmp = command;
    // bool filtered = false;
    size_t command_len = strlen(command) - 1;
    while (isspace((unsigned char)*tmp)) {
        tmp++;
        if (*tmp == '\0') {
            return string_vector_create();
        }
    }


        while (command_len > 0 && isspace(tmp[command_len])) {
        command_len --;
    }
        char buffer[command_len + 2];
        strncpy(buffer, command, command_len + 1);
        buffer[command_len + 1] = '\0';

        sstring* tmp_sstring = cstr_to_sstring(buffer);
        vector* store_sstring = sstring_split(tmp_sstring, ' ');
        size_t size_after_erase = 0;
        sstring_destroy(tmp_sstring);
        while (size_after_erase < vector_size(store_sstring)) {
            if (strcmp(vector_get(store_sstring, size_after_erase), "")== 0) {
                vector_erase(store_sstring, size_after_erase);
            } else {
                size_after_erase++;
            }
        }
        return store_sstring;
    }

// ps
//     process_info* create_ps(size_t n) {
    //     print_process_info_header();
    //     process_info* info = (process_info*)calloc(1, sizeof(process_queue));
    //         info -> pid = info_p;
    //         info -> command = (char*)calloc(strlen(command) + 1, sizeof(char));
    //         strcpy(info -> command, command);

    //         char buffer[40];
    //         snprintf(buffer, 40, "proc/%d/status", info -> pid);
    //         FILE* file = fopen(buffer, "r");
        
    //     for (size_t i = 0; i < vector_size(process_queue); i++) {
    //         process* p = (process*) vector_get(process_queue, i);
    //         unsigned long ctime, stime;
    //     unsigned long long startt;
    //     fscanf(file, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu ", &(info -> state));
    //     fscanf(file, "%*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld ", &ctime, &stime, &(info -> nthreads));
    //     fscanf(file, "%*ld %llu %lu", &startt, &(info -> vsize));

    //     char s_buffer[TIME_BASE];
    //     time_t rtime = p -> time;
    //     struct tm* t_info = localtime(&rtime);
    //     time_struct_to_string(s_buffer, TIME_BASE, t_info);
    //     info -> start_str = s_buffer;

    //     char temp[TIME_BASE];
    //     size_t sstime = (ctime + stime) / sysconf(_SC_CLK_TCK);
    //     execution_time_to_string(temp, TIME_BASE, sstime / 60, sstime % 60);
    //     info -> time_str = temp;
    //     }
            
    //         if (!file) {
    //             print_script_file_error();
    //             exit(1);
    //         }
    //         char thread[500];
    //         char* vsize;
    //         while (fgets(thread, MAX_CHAR_NUM, file) != NULL) {
    //             if  (!strncmp(thread, "Threads:", 8)) {
    //                 vsize = thread + 9;
    //                 char* tmp_thread;
    //                 while (isspace(*vsize)) {
    //                     vsize++;
    //                 }
    //                 info -> nthreads = strtol(vsize, &tmp_thread, BASE_NUM);
    //             } else if (!strncmp(thread, "VmSize:", 7)) {
    //                 vsize = thread + 8;
    //                 char* tmp_vsize;
    //                 while (isspace(*vsize)) {
    //                     vsize++;
    //                 }
    //                 info -> vsize = strtol(vsize, &tmp_vsize, BASE_NUM);
    //             } else if (!strncmp(thread, "State:", 6)) {
    //                 vsize = thread + 7;
    //                 while(isspace(*vsize)) {
    //                     vsize++;
    //                 }
    //                 info -> state = *vsize;
    //             }
    // } 
    // fclose(file);
    // return info;
//         return info;
//  }

void process_ps() {
    print_process_info_header();
    for (size_t i = 0; i < vector_size(process_queue); i++) {
        char buffer[MAX_CHAR_NUM];
        process *p = vector_get(process_queue, i);
        unsigned long utime, stime;
        unsigned long long startt;
        process_info* info = (process_info*)malloc(sizeof(process_info));
        char s_buffer[TIME_BASE];
        char t_buffer[TIME_BASE];

        sprintf(buffer, "/proc/%d/stat", p->pid);
        FILE *file = fopen(buffer, "r");
        if (!file) {
            continue; 
        }
        
        info -> pid = p -> pid;
        info -> command = p->command;
        
        fscanf(file, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu ", &(info -> state));
        fscanf(file, "%*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld ", &utime, &stime, &(info -> nthreads));
        fscanf(file, "%*ld %llu %lu", &startt, &(info -> vsize));  
        
        info -> vsize = info -> vsize /1024;
        
        time_t t = p->time;
        struct tm *info_t = localtime(&t);
        time_struct_to_string(s_buffer, TIME_BASE, info_t);
        info -> start_str = s_buffer;
        
        size_t sum = (utime + stime) / sysconf(_SC_CLK_TCK);
        execution_time_to_string(t_buffer, TIME_BASE, sum / 60, sum % 60);
        info -> time_str = t_buffer;
        print_process_info(info);
        fclose(file);

        // free(info -> command);
        // // free(info -> nthreads);
        // free(info -> start_str);
        // free(info -> time_str);
        // // free(info -> state);
        // // free(info -> vsize);
        // free(info);
    }
}

//Signal Commands

    //kill
    void kill_process(char* str, char* command) {
        pid_t pid;
        if (!str || !strlen(str) || sscanf(str, "%d", &pid)) {
            print_invalid_command(command);
            return;
        }

        
        if (kill(pid, SIGTERM) == 0) {
            print_killed_process(pid, command);
            free_process(pid);
            return;
        }
        print_no_process_found(pid);
    }


    //stop
    void stop_process(char* str, char* command) {
        pid_t pid;
        if (!str || !strlen(str) || sscanf(str, "%d", &pid)) {
            print_invalid_command(command);
            return;
        }

        
        if (kill(pid, SIGSTOP) == 0) {
            print_stopped_process(pid, command);
            return;
        }
        print_no_process_found(pid);
    }

    //cont
    void cont_process(char* str, char* command) {
       pid_t pid;
        if (!str || !strlen(str) || sscanf(str, "%d", &pid)) {
            print_invalid_command(command);
            return;
        }

        
        if (kill(pid, SIGCONT) == 0) {
            return;
        } else {
            print_no_process_found(pid);
        }
    }

    // pdf
    // void pdf(pid_t pid, char* command) {
    //     pid_t pid = getpid();
    //     process* p = create_ps("./shell", pid);
    //     vector_push_back(process_queue, p);

    // }

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    signal(SIGCHLD, background_signal);
    signal(SIGINT, ctrlc_signal);
    char *file_n = NULL;
    char *history_n = NULL;
    char* read_line = NULL;
    size_t len = 0;
    FILE *input = stdin;
    FILE *history_f = NULL;
    int opt;
    int c = 1;
    
    process_queue = shallow_vector_create();
    process shell_p;
    shell_p.command = argv[0];
    shell_p.pid = getpid();
    shell_p.time = time(NULL);
    vector_push_back(process_queue, &shell_p);

    // -f -h
    while ((opt = getopt(argc, argv, "f:h:")) != -1) {
      switch(opt){
        case 'h': history_n = optarg; 
            c+=2; 
            break;
        case 'f': file_n = optarg; 
            c+=2; 
            break;
      }
    }

    // usage
    if (argc != c) {
      print_usage();
      return 1;
    }

    // history file
    // FILE *file_h;
    // char *history_path;
    if (history_n) {
        history_f = fopen(history_n, "a+");
        // file_h = fopen(history_path, "r");
        if (!history_f) {
            print_history_file_error();
            return 1;
        }
    }

    // script file error
    if (file_n) {
        input = fopen(file_n, "r");
        if (!input) {
            print_script_file_error();
            return 1;   
        }
    }

    vector* history = read_history(history_f);
    char buffer[MAX_BUFFER_SIZE];

    // prompt
    print_prompt(getcwd(buffer, MAX_BUFFER_SIZE), getpid());
    fflush(stdout);
    while(getline(&read_line, &len, input) >= 0) {
        // if (read_byte < 0) {
        //     break;
        // }

        // char* paths = get_full_path("./");
        // print_prompt(paths, getpid());
        // free(paths);
        
        read_line[strlen(read_line) - 1] = '\0';
        if (file_n) {
            print_command(read_line);
            fflush(stdout);
        }
    
        vector* line_wo_space = filter_space(read_line);
        // bool is_empty = false;
        // size_t tmp_len = 0;

        // while (*tmp_line == ' ') {
        //     tmp_line++;
        //     if (*tmp_line == '\0') {
        //         line_wo_space = string_vector_create();
        //         is_empty = true;
        //         break;
        //     }
        // }

        // // sstring to pick out space
        // if (!is_empty) {
        //     tmp_len = strlen(tmp_line) - 1;
        //     while (strlen(tmp_line) - 1 > 0 && *tmp_line == ' ') {
        //         tmp_len --;
        //     }

        //     char tmp_buffer[tmp_len + 2];
        //     strncpy(tmp_buffer, tmp_line, strlen(tmp_line) + 1);
        //     tmp_buffer[strlen(tmp_line + 1)] = '\0';

        //     sstring* tmp_sstring = cstr_to_sstring(tmp_buffer);
        //     vector* store_sstring = sstring_split(tmp_sstring, ' ');
        //     free(tmp_sstring);
            
        //     size_t size_after_erase = 0;
        //     while (size_after_erase < vector_size(store_sstring)) {
        //         if (strcmp(vector_get(store_sstring, size_after_erase), "")== 0) {
        //             vector_erase(store_sstring, size_after_erase);
        //         } else {
        //             size_after_erase++;
        //         }
        //     }
        //     line_wo_space = store_sstring;
        // }

        size_t new_size = vector_size(line_wo_space);
        
        // check directory
        char* a = (char*)vector_get(line_wo_space, 0);
        if (new_size <= 0) {
            if (check_built_in_command(line_wo_space)) {

            }
        }
            else if (strcmp(a, "exit") == 0) {
                    vector_destroy(line_wo_space);
                    break;
                }
            else if (strcmp(a, "!history") == 0) {
                    for (size_t i = 0; i < vector_size(history); i++) {
                        print_history_line(i, vector_get(history, i));
                    }
                } else if (strcmp(a, "cd") == 0) {
                    write_history(history_f, read_line, history);
                    if (new_size == 1) {
                        print_no_directory("");
                    } else if (chdir(vector_get(line_wo_space, 1)) == -1) {
                    print_no_directory(vector_get(line_wo_space, 1));
                }
                    
                }  else if (a[0] == '!') {
                    char* prefix = read_line + 1;
                    int tmp = -1;
                    // bool found = false;
                    int i;
                    for (i = (int)vector_size(history) - 1; i >= 0; i--) {
                        if  (strlen(vector_get(history, i)) < strlen(prefix)) {
                            continue;
                        }
                        if (strncmp(vector_get(history, i), prefix, strlen(prefix)) == 0) {
                            tmp = i;
                            break;
                        }
                    }
                    
                    if (tmp != -1) {

                        print_command(vector_get(history, tmp));
                        write_history(history_f, vector_get(history, tmp), history);
                        
                        // get history
                        vector* match_history;
                        match_history = filter_space(vector_get(history, tmp));
                        fflush(stdout);
                        fflush(input);
                        f_e_w_line(match_history);
                        vector_destroy(match_history);
                    } else {
                        print_no_history_match();
                    }
                } else if (a[0] == '#') {
                    size_t curr_pos;
                    if (sscanf(a, "#%zu", &curr_pos) && vector_size(history) > curr_pos) {
                        
                            print_command(vector_get(history, curr_pos));
                            write_history(history_f, vector_get(history, curr_pos), history);
                            
                            vector* get_input_space;
                            get_input_space = filter_space(vector_get(history, curr_pos));
                            fflush(stdout);
                            fflush(input);
                            f_e_w_line(get_input_space);
                            vector_destroy(get_input_space);
                    } else {
                            print_invalid_index();
                        }
                }
                //ps
                else if (!strcmp(a, "ps")) {
                    process_ps();
                }
                //kill
                else if (strcmp(a, "kill") == 0) {
                    // pid_t kill_p;
                    kill_process(vector_get(line_wo_space, 1), read_line);
                }
                //stop
                else if (strcmp(a, "stop") == 0) {
                    // pid_t stop_p;
                    stop_process(vector_get(line_wo_space, 1), read_line);
                }
                //cont
                else if (strcmp(a, "cont") == 0) {
                    // pid_t cont_p;
                    cont_process(vector_get(line_wo_space, 1), read_line);
                }
                 else {
                    write_history(history_f, read_line, history);
                    fflush(input);
                    fflush(stdout);
                    f_e_w_line(line_wo_space);
                }
                vector_destroy(line_wo_space);
                line_wo_space = NULL;
                free(read_line);
                read_line = NULL;
                print_prompt(getcwd(buffer, MAX_BUFFER_SIZE),getpid());
            }
            free(read_line);
            read_line = NULL;
            vector_destroy(history);
            return 0;
}

            
    


