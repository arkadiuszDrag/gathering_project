#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define NANO 480000000L
#define EXIT_PIPE_ERROR 69

struct sysinfo i; 
struct timespec monotime;
char* calc_time(time_t bootime);

typedef struct __attribute__((__packed__)) raport{
    short val;
    pid_t PID;
} raport;

int main(int argc, char** argv){
    sysinfo(&i); 
    const time_t boottime = time(NULL) - i.uptime;

    int pipe_fd_w[2], pipe_fd_r[2];

    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = NANO;

    if(pipe(pipe_fd_w) == -1){
        exit(EXIT_PIPE_ERROR);
    }

    if(pipe(pipe_fd_r) == -1){
        exit(EXIT_PIPE_ERROR);
    }

    pid_t pidc, pid;
    int c, blok, wolumen;
    int child_count;
    int data_fd, raport_fd, success_fd; 
    int len_file;
    char* next;

    double len_success;

    while((c = getopt(argc, argv, "s:d:w:f:l:p:")) != -1) {
		switch (c) {
            case 'w':
                blok = (int) strtol (optarg, &next, 10);
    
                if (next == optarg) {  //https://stackoverflow.com/questions/17292545/how-to-check-if-the-input-is-a-number-or-not-in-c
                    exit(13);
                }

                if(!strcmp("Ki", next))
                    blok *= 1024;

                else if(!strcmp("Mi", next))
                    blok *= 10242;

                break;

            case 's':
                wolumen = (int) strtol (optarg, &next, 10);
    
                if (next == optarg) {  //https://stackoverflow.com/questions/17292545/how-to-check-if-the-input-is-a-number-or-not-in-c
                    exit(13);
                }

                if(!strcmp("Ki", next))
                    wolumen *= 1024;

                else if(!strcmp("Mi", next))
                    wolumen *= 10242;

                break;

            case 'p':
                child_count = (int)(strtol(optarg, &next, 10));
                break;

            case 'd':
                if((data_fd = open(optarg, O_RDONLY) )< 0)
                    exit(EXIT_FAILURE);

                len_file = lseek(data_fd, 0, SEEK_END);
                lseek(data_fd, 0, SEEK_SET);

                break;

            case 'l':
                if((raport_fd = open(optarg, O_WRONLY) )< 0)
                    exit(EXIT_FAILURE);

                break;

            case 'f':
                if((success_fd = open(optarg, O_RDWR)) < 0)
                    exit(EXIT_FAILURE);

                len_success = lseek(success_fd, 0, SEEK_END);
                lseek(success_fd, 0, SEEK_SET);
                break;

            default:
                perror("Wrong argument. \n");
                break;
        }
    }

    if(len_file < 2*wolumen){
        perror("Size of file with data is too small. \n");
        exit(EXIT_FAILURE);
    }

    char bufc[2*blok];

    int length = snprintf(NULL, 0, "%d", blok);
    char str[length]; 
    sprintf(str, "%d", blok); 

    char* args[] = {"poszukiwacz2", str, NULL};

    double len_file_act = 0;

    int status;

    int PERCENT_OF_FILE = 1;
    int ERROR_READ = 0;
    int ERROR_CHILD_DEATH = 0;

    int child_count_act = 0;

    while (PERCENT_OF_FILE){

        if(ERROR_READ && ERROR_CHILD_DEATH)
            nanosleep(&tim, NULL);
            
        while(child_count_act < child_count){

            pid = fork();

            switch(pid){
                case -1:
                    perror("Fork error \n");
                    exit(EXIT_FAILURE);

                case 0:
                    clock_gettime(CLOCK_MONOTONIC, &monotime);
                    char* time_to_write = calc_time(boottime);
                        
                    int length = snprintf(NULL, 0, "%d", getpid());
                    char* str = malloc(length + 1);
                    snprintf(str, length + 1, "%d", getpid()); 
                    char buff_raport[50];
                    snprintf(buff_raport, 50, "%-24s %-12s %-7d", time_to_write, "utworzenie", getpid());
                    
                    write(raport_fd, buff_raport, strlen(buff_raport));
                    
                    dup2(pipe_fd_w[0], 0); 
                    close(pipe_fd_w[0]);
                    close(pipe_fd_w[1]);
                    
                    dup2(pipe_fd_r[1], 1);
                    close(pipe_fd_r[0]);
                    close(pipe_fd_r[1]);
                    
                    execvp("poszukiwacz", args);
                    _exit(0);

                    break;

                default:
                    read(data_fd, bufc, 2*blok);

                    write(pipe_fd_w[1], bufc, 2*blok);

                    int cur = lseek(data_fd, 0, SEEK_CUR);

                    if(cur + 2*blok > 2*wolumen){
                        exit(EXIT_PIPE_ERROR);
                    }

                    if(fcntl(pipe_fd_r[0], F_SETFL, O_NONBLOCK) < 0)
                        exit(EXIT_FAILURE);
                    
                    raport* buff_r = malloc(sizeof(raport));
                    int help;
                    
                    while((help = read(pipe_fd_r[0], &buff_r->val, sizeof(buff_r->val))) != 0){
                        if(len_file_act/len_success >= 0.75){
                            PERCENT_OF_FILE = 0;
                            break;
                        }
                        
                        if(help < 0){
                            ERROR_READ = 1;
                            break;
                        }

                        read(pipe_fd_r[0], &buff_r->PID, sizeof(buff_r->PID));
                        
                        lseek(success_fd, ((buff_r->val * 4 ) - 4), SEEK_SET);

                        pid_t x;

                        read(success_fd, &x, sizeof(x));

                        if(!x){
                            int cur = (lseek(success_fd, 0, SEEK_CUR) - 4);
                            lseek(success_fd, cur, SEEK_SET);
                            write(success_fd, &buff_r->PID, sizeof(buff_r->PID));
                            len_file_act += 4;
                        }
                    }

                    break;   
                
                }
        child_count_act++;
    }

        if(((pidc = waitpid(0, &status, 0)) > 0)){
            child_count_act--;
            if (WIFEXITED(status) )
            {      
                if(WEXITSTATUS(status) == EXIT_PIPE_ERROR){
                    exit(EXIT_PIPE_ERROR);
                }
                else if(WEXITSTATUS(status) > 10){
                    if(child_count > 1){
                        child_count--;
                    }
                    else{
                        exit(EXIT_FAILURE);
                    }
                    ERROR_CHILD_DEATH = 1;
                }

                clock_gettime(CLOCK_MONOTONIC, &monotime);
                char* time_to_write = calc_time(boottime);
                    
                int length = snprintf(NULL, 0, "%d", pidc);
                char* str = malloc(length + 1);
                snprintf(str, length + 1, "%d", pidc); 
                char buff_raport[50];
                snprintf(buff_raport, 50, "%-24s %-12s %-7d", time_to_write, "zakonczenie", pidc);
                write(raport_fd, buff_raport, strlen(buff_raport));
            }  
        }
    }
    
    return 0;
}

char* calc_time(time_t boottime){
    clock_gettime(CLOCK_MONOTONIC, &monotime);
    time_t curtime = boottime + monotime.tv_sec;

    char* time_to_write = ctime(&curtime);

    return time_to_write;
}
