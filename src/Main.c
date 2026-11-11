#include <stdio.h>     // printf, fprintf, fgets, ferror, perror, stdin, stderr, NULL
#include <stdlib.h>    // exit
#include <string.h>    // strlen, strcmp
#include <unistd.h>    // dup2, fork, execl, close, ftruncate, STDOUT_FILENO
#include <fcntl.h>     // open, shm_open, O_WRONLY, O_CREAT, O_TRUNC, O_RDWR
#include <sys/stat.h>  // S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH
#include <sys/mman.h>  // mmap, PROT_READ, PROT_WRITE, MAP_SHARED, MAP_FAILED
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <semaphore.h> // sem_t, sem_init, sem_wait, sem_post
#include <stdbool.h>   // true

#include "SharedData.h"

void DuplicateTo(int oldFd, int newFd)
{
    if (dup2(oldFd, newFd) == -1)
    {
        perror("dup2 error");
        exit(1);
    }
}

int OpenFile()
{
    char outpufFile[256];

    printf("Enter output file name: ");

    if (fgets(outpufFile, sizeof(outpufFile), stdin) != NULL)
    {
        size_t length = strlen(outpufFile);

        if (length > 0 && outpufFile[length - 1] == '\n')
        {
            outpufFile[length - 1] = '\0';
        }
    }
    else if (ferror(stdin))
    {
        perror("File name reading error");
        exit(1);
    }

    if (outpufFile[0] == '\0')
    {
        fprintf(stderr, "Error: file name is empty\n");
        exit(1);
    }

    int outputFileId = open(
        outpufFile,
        O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (outputFileId == -1)
    {
        perror("File opening error");
        exit(1);
    }
    return outputFileId;
}

int main()
{
    const char *memoryName = "/lab4_shared_memory";

    int descriptor = shm_open(
        memoryName,
        O_CREAT | O_RDWR,
        S_IRUSR | S_IWUSR);

    if (descriptor == -1)
    {
        perror("shm_open error");
        return 1;
    }

    if (ftruncate(descriptor, sizeof(SharedData)) == -1)
    {
        perror("ftruncate error");
        shm_unlink(memoryName);
        return 1;
    }

    SharedData *shared = mmap(
        NULL,
        sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        descriptor,
        0);

    if (shared == MAP_FAILED)
    {
        perror("mmap error");
        shm_unlink(memoryName);
        return 1;
    }

    close(descriptor);

    if (shared == MAP_FAILED)
    {
        perror("mmap error");
        shm_unlink(memoryName);
        return 1;
    }

    if (sem_init(&shared->parentCanWork, 1, 1) == -1 || sem_init(&shared->childCanWork, 1, 0) == -1)
    {
        perror("sem_init error");
        munmap(shared, sizeof(SharedData));
        shm_unlink(memoryName);
        return 1;
    }

    int outputFileId = OpenFile();

    pid_t processId = fork();

    if (processId == -1)
    {
        perror("Fork error");
        return 1;
    }

    if (processId == 0)
    {
        DuplicateTo(outputFileId, STDOUT_FILENO);
        close(outputFileId);

        execl("./child", "child", memoryName, (char *)NULL);

        perror("Child execution error");
        return 1;
    }

    close(outputFileId);

    sem_wait(&shared->parentCanWork);
    while (true)
    {
        printf("Enter text: ");

        if (fgets(shared->textBuffer, sizeof(shared->textBuffer), stdin) == NULL)
        {
            strcpy(shared->textBuffer, "/esc\n");
            sem_post(&shared->childCanWork);
            break;
        }

        sem_post(&shared->childCanWork);

        // child job

        sem_wait(&shared->parentCanWork);

        if (strcmp(shared->textBuffer, "/esc\n") == 0)
        {
            break;
        }

        if (shared->textBuffer[0] != '\n')
        {
            printf("Validation error: %s", shared->textBuffer);
        }
    }

    if (wait(NULL) == -1)
    {
        perror("wait error");
    }

    sem_destroy(&shared->parentCanWork);
    sem_destroy(&shared->childCanWork);
    munmap(shared, sizeof(SharedData));
    shm_unlink(memoryName);

    return 0;
}