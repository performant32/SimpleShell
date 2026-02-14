#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define true 1
#define false 0

typedef struct CStrVec{
    char** data;
    int size;
} CStrVec;

void CStrVecPushBack(CStrVec* vec, char* data){
    vec->data = realloc(vec->data, sizeof(data) * ++vec->size);
    vec->data[vec->size - 1] = data;
}

void CStrVecCleanUp(CStrVec* vec){
    for(int i = 0; i < vec->size; i++)
        free(vec->data[i]);
    free(vec->data);
    *vec = (CStrVec){NULL,0};
}
int directory_exists(const char* path){
    struct stat s;
    int err = stat(path, &s);

    if(err == -1){
        return -1;
    }

    return S_ISDIR(s.st_mode) ? 0 : 1;
}
int main(int argc, char** argv){
    const char* path = getenv("PATH");
    if(!path){
        fprintf(stderr, "Failed to get path environmental variable");
        return -1;
    }
    int inputBufferLength = 1000;
    char* userInputBuffer = calloc(1,inputBufferLength);
    int lastStatus = 0;

    CStrVec environment={};
    CStrVecPushBack(&environment, "TERM=alacritty");
    printf("shell: ");
    fflush(stdout);
    while((fgets(userInputBuffer, inputBufferLength, stdin)) > 0){
        int l = strlen(userInputBuffer);
        int b1 = l - 1;
        for(int i = 0; i < l; i++){
            char c = userInputBuffer[i];
            if(c == ' '){
                b1 = i;
                break;
            }
        }
        char* argv1 = malloc(b1 + 1);
        memcpy(argv1, userInputBuffer, b1);
        argv1[b1] = 0;
        if(strcmp(argv1, "exit") == 0){
            free(argv1);
            break;
        }
        CStrVec vec={NULL,0};
        CStrVecPushBack(&vec, argv1);
        int lastStart = b1 + 1;
        int i = 0;
        for(i = lastStart; i< l; i++){
            char c = userInputBuffer[i];
            if(c == ' '){
                if(i - lastStart > 1){
                    int argLen = i - lastStart;
                    char* arg = malloc(argLen + 1);
                    arg[argLen] = 0;
                    memcpy(arg, userInputBuffer + lastStart, argLen);
                    CStrVecPushBack(&vec, arg);        
                }
                lastStart = i + 1;
            }
        }
        
        if(i - lastStart > 1){
            int argLen = (i - lastStart) - 1;
            char* arg = malloc(argLen + 1);
            arg[argLen] = 0;
            memcpy(arg, userInputBuffer + lastStart, argLen);
            CStrVecPushBack(&vec, arg);        
        }
        CStrVecPushBack(&vec, NULL);
        const char* programName = argv1;
        int programNameLen = strlen(programName);
        int pathLen = strlen(path);
        lastStart = 0;
        int locatedProgram = 0;
        for(i = 0; i < pathLen; i++){
            char c = path[i];
            if(c != ':')continue;
            if(i - lastStart < 1)continue;
            int newLen = i - lastStart;
            
            char* str = malloc(newLen + 1);
            str[newLen] = 0;
            memcpy(str, path + lastStart, newLen);
            lastStart = i + 1;

            if(directory_exists(str) != 0){
                //printf("DIr doesnt exists %s\n", str);
                goto clean;
            }
            int len = newLen + programNameLen + 2;
            char* programPath = malloc(len);
            int programOffset = newLen;
            if(str[newLen] != '/'){
                programPath[programOffset] = '/';
                programOffset += 1;
            }
            memcpy(programPath, str, newLen);
            memcpy(programPath + programOffset, programName, programNameLen);
            FILE* program = fopen(programPath, "r");
            if(!program)goto clean;
            locatedProgram = true;
            fflush(stdout);
            int pid = fork();
            if(pid == 0){
                execve(programPath, vec.data, environment.data);
            } 
            fclose(program);
            waitpid(-1, &lastStatus, 0);
        clean:
            free(str);
        }
        if(!locatedProgram)
            printf("%s is not recognized as a command\n", argv1);
        CStrVecCleanUp(&vec);
        printf("shell: ");
        fflush(stdout);
    }
}
