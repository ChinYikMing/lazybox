#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

typedef void(*link_handler)(char *path, char *link_name);

typedef struct stat Stat;
typedef struct passwd Passwd;
typedef struct group Group;
typedef struct timespec ts;

#define ROOT 0

void symbolic_link_handler(char *path, char *link_name);
void hard_link_handler(char *path, char *link_name);

void my_stat(char *command);

void my_chown(char *command);

void my_cd(char *command);

void my_ln(char *command);

time_t get_timespec_sec(ts *t);

int prompt = 0;

int main(int argc, char **argv){
    char *exec_name = basename(argv[0]);
    char command[4096];
    if(strcmp(exec_name, "lazyshell") == 0 || strcmp(exec_name, "lazybox") == 0){
        while(1){
            printf("lazybox$");
            fgets(command, 4096, stdin);
            if(command && command[strlen(command) - 1] == '\n')
                command[strlen(command) - 1] = '\0';
            if(strcmp(command, "exit") == 0){
                exit(EXIT_SUCCESS);
            } else {
                char cmd[128];
                char *opt = strchr(command, ' ');
                if(opt){
                    strncpy(cmd, command, opt - command);
                    if(strcmp(cmd, "cd") == 0)
                        my_cd(command);
                    else if(strcmp(cmd, "ln") == 0)
                        my_ln(command);
                    else if(strcmp(cmd, "stat") == 0)
                        my_stat(command);
                    else if(strcmp(cmd, "chown") == 0)
                        my_chown(command);
                    else
                        system(command);
                } else {
                    system(command);
                }
            }
        }
        exit(EXIT_SUCCESS);
    } 

    if(argc >= 2){
        for(int i = 1; i < argc; ++i){
            strcat(command, " ");
            strcat(command, argv[i]);
        }
    }
    
    if(strcmp(exec_name, "ln") == 0){
        my_ln(command);
    }  else if(strcmp(exec_name, "cd") == 0){
        my_cd(command);
    }  else if(strcmp(exec_name, "stat") == 0){
        my_stat(command);
    }  else if(strcmp(exec_name, "chown") == 0){
        my_chown(command);
    }  else {
        strcpy(command, exec_name);

        /* parse option */
        if(argc >= 2){
            for(int i = 1; i < argc; ++i){
                strcat(command, " ");
                strcat(command, argv[i]);
            }
        }
        system(command);
    }
    
    exit(EXIT_SUCCESS);
}

void symbolic_link_handler(char *path, char *link_name){
    if(symlink(path, link_name) == -1){
        perror("Error");
        exit(EXIT_FAILURE);
    }
    return;
}
void hard_link_handler(char *path, char *link_name){
    if(link(path, link_name) == -1){
        perror("Error");
        exit(EXIT_FAILURE);
    }
    return;
}

void my_stat(char *command){
    Stat statbuf;
    uid_t uid;
    time_t atime, mtime, v_ctime;
    Passwd *passwd;
    char pathname[1024];
    char *ptr = strchr(command, ' ');
    strcpy(pathname, ++ptr);

    if(stat(pathname, &statbuf) == -1){
        perror("Error");
        exit(EXIT_FAILURE);
    }
    atime = statbuf.st_atime;
    mtime = statbuf.st_mtime;
    v_ctime = statbuf.st_ctime;
    uid = statbuf.st_uid;

    passwd = getpwuid(uid);

    printf("fileowner: %s\n", passwd->pw_name);
    char *atime_str = ctime(&atime);
    printf("atime: %s", atime_str);
    char *mtime_str = ctime(&mtime);
    printf("mtime: %s", mtime_str);
    char *ctime_str = ctime(&v_ctime);
    printf("ctime: %s", ctime_str);
    return;
}

void my_chown(char *command){
    assert(setuid(ROOT) == 0);

    int ret;
    ts file_t, current_t;
    Stat statbuf;
    ret = stat("sudo_success", &statbuf);
    /* first time to chown, no sudo_access exists */
    if(ret == -1 && errno == ENOENT){
        prompt = 1;
    } else {
        file_t = statbuf.st_ctim;
        clock_gettime(CLOCK_REALTIME, &current_t);
        time_t time1 = get_timespec_sec(&current_t);
        time_t time2 = get_timespec_sec(&file_t);
        // printf("time1: %zu, time2: %zu\n", time1, time2);
        if(get_timespec_sec(&current_t) - get_timespec_sec(&file_t) > 60){
            prompt = 1;
        }
    }

    if(prompt){
        printf("你要執行的是特權指令，確定的話輸入『yes』否則按下『enter』終止操作\n");
        char str[64];
        fgets(str, 64, stdin);
        if(str && str[strlen(str) - 1] == '\n')
            str[strlen(str) - 1] = 0;
        if(strlen(str) < 3 || strcmp(str, "yes") != 0)
            return;
        system("touch sudo_success");
    } 

    /* sudo will remain 60 seconds */
    prompt = 0;

    char uid_buf[1024];
    char pathname[4096];
    char *ptr = strchr(command, ' ');
    char *qtr = strrchr(command, ' ');
    strncpy(uid_buf, ++ptr, qtr - ptr);
    uid_buf[qtr - ptr] = 0;
    strcpy(pathname, ++qtr);

    uid_t uid = (uid_t)strtol(uid_buf, NULL, 10);

    if(chown(pathname, uid, -1) == -1){
        printf("filename: %s\n", pathname);
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void my_cd(char *command){
    char cmd[1024];
    char path[4096];
    char *ptr = strchr(command, ' ');
    strncpy(cmd, command, ptr - command);
    strcpy(path, ptr + 1);

    /* use chdir to implement cd command */
    if(chdir(path) != 0){
        perror("Error");
        exit(EXIT_FAILURE);
    }
}

void my_ln(char *command){
    char *ptr = strchr(command, ' ');
    ptr++;

    link_handler lh;
    char path[4096];
    char link_name[1024];
    char *qtr;
    // printf("ptr: %s\n", ptr);
    if(*ptr == '-'){
        lh = symbolic_link_handler;
        ptr += 3;
    } else {
        lh = hard_link_handler;
    }
    qtr = strchr(ptr, ' ');
    strncpy(path, ptr, qtr - ptr);
    path[qtr-ptr] = 0;
    strcpy(link_name, ++qtr);

    // printf("path: %s, link: %s\n", path, link_name);
    lh(path, link_name);
}

time_t get_timespec_sec(ts *t){
    long sec = (long)t->tv_sec;
    long nanosec = t->tv_nsec;
    
    sec += (nanosec / 1000000000);

    return (time_t)sec;
}
