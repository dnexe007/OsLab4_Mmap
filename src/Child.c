#include <stdio.h>     // fprintf, sprintf, stderr, stdout, NULL
#include <string.h>    // strcmp, strcpy
#include <ctype.h>     // isupper
#include <stdbool.h>   // true
#include <fcntl.h>     // shm_open, O_RDWR
#include <sys/stat.h>  // S_IRUSR, S_IWUSR
#include <sys/mman.h>  // mmap, PROT_READ, PROT_WRITE, MAP_SHARED
#include <semaphore.h> // sem_wait, sem_post
#include <unistd.h>    // close
#include "SharedData.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Child error: missing argument\n");
        return 1;
    }

    const char *memoryName = argv[1];

    int descriptor = shm_open(
        memoryName,
        O_RDWR,
        S_IRUSR | S_IWUSR);

    if (descriptor == -1)
    {
        perror("Child shm_open error");
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
        perror("Child mmap error");
        close(descriptor);
        return 1;
    }

    close(descriptor);

    while (true)
    {
        sem_wait(&shared->childCanWork);

        if (strcmp(shared->textBuffer, "/esc\n") == 0)
        {
            sem_post(&shared->parentCanWork);
            munmap(shared, sizeof(SharedData));
            return 0;
        }

        if (isupper(shared->textBuffer[0]))
        {
            fprintf(stdout, "%s", shared->textBuffer);
            strcpy(shared->textBuffer, "\n");
        }

        else
        {
            if (shared->textBuffer[0] == '\n')
            {
                strcpy(
                    shared->textBuffer,
                    "input is empty\n");
            }

            else
            {
                sprintf(
                    shared->textBuffer,
                    "input starts with '%c' - not uppercase\n",
                    shared->textBuffer[0]);
            }
        }
        sem_post(&shared->parentCanWork);
    }
}
