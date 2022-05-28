#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

typedef struct __attribute__((__packed__)) raport{
    short val;
    pid_t PID;
} raport;

int function(int blok);

int main(int argc, char* argv[]){
    if(argc != 2){
        exit(11);
    }

    struct stat stat;
    if((S_ISFIFO(stat.st_mode) < 0 ) && fstat(0, &stat)){
        exit(12);
    }

    char* next;
    int blok = (int) strtol (argv[1], &next, 10);

    if ((next == argv[1])) {  //https://stackoverflow.com/questions/17292545/how-to-check-if-the-input-is-a-number-or-not-in-c
        exit(13);
    }

    if(!strcmp("Ki", next))
        blok *= 1024;

    else if(!strcmp("Mi", next))
        blok *= 10242;

    int repeat = function(blok);
    double repeats = (double) repeat / blok;
    
    int status = (int)(repeats * 100);
    status /= 10 + 1;

    exit(status);
}

int function(int blok){
    raport* raport_gen = malloc(sizeof(raport));

    if(raport_gen == NULL){
        exit(17);
    }

    short x, val_to_compare[blok];
    int i = 0, send_raport = 1, repeat = 0;

    while( i<blok ){
        if(read(0, &x, 2) <= 0){
            exit(14);
        }

        val_to_compare[i] = x;
        send_raport = 1;

        int k = 0;
        
       while(k<i){
            if(val_to_compare[k] == val_to_compare[i]){
                repeat += 1;
                send_raport = 0;
                break;
            }

            k++;
        }

        if(send_raport){           
            raport_gen->val = val_to_compare[i];
            raport_gen->PID = getpid();
            write(1, raport_gen, sizeof(raport)); 
        }   
        i++; 
    }

    free(raport_gen);
    return repeat;
}