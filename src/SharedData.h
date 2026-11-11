#include <semaphore.h> // sem_t

typedef struct
{
    char textBuffer[1024];
    sem_t parentCanWork;
    sem_t childCanWork;
} SharedData;