/***********************************************************************************/
//***********************************************************************************
//            *************NOTE**************
// This is a template for the subject of RTOS in University of Technology Sydney(UTS)
// Please complete the code based on the assignment requirement.

//***********************************************************************************
/***********************************************************************************/

/*
   To compile assign2_template-v3.c ensure that gcc is installed and run 
   the following command:

   gcc your_program.c -o your_ass-2 -lpthread -lrt -Wall
   */

/* make assign2 data.txt output.txt */

#include  <pthread.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <stdio.h>
#include  <sys/types.h>
#include  <fcntl.h>
#include  <string.h>
#include  <sys/stat.h>
#include  <semaphore.h>
#include  <sys/time.h>
#include <sys/mman.h>

/* to be used for your memory allocation, write/read. man mmsp */
#define SHARED_MEM_NAME "/my_shared_memory"
#define SHARED_MEM_SIZE 1024

/* --- Structs --- */
typedef struct ThreadParams {
  int pipeFile[2]; // [0] for read and [1] for write. use pipe for data transfer from thread A to thread B
  sem_t sem_A, sem_B, sem_C; // the semphore
  char message[255];
  char inputFile[100]; // input file name
  char outputFile[100]; // output file name
} ThreadParams;

/* Global variables */
int sum = 1;

pthread_attr_t attr;

int shm_fd;// use shared memory for data transfer from thread B to Thread C 

/* --- Prototypes --- */

/* Initializes data and utilities used in thread params */
void initializeData(ThreadParams *params);

/* This thread reads data from data.txt and writes each line to a pipe */
void* ThreadA(void *params);

/* This thread reads data from pipe used in ThreadA and writes it to a shared variable */
void* ThreadB(void *params);

/* This thread reads from shared variable and outputs non-header text to src.txt */
void* ThreadC(void *params);

/* Read contents of specified file pointer */
int read_file(char filename[], void* shm_ptr);

/* --- Main Code --- */
int main(int argc, char const *argv[]) {


  /* Verify the correct number of arguments were passed in */
  if (argc != 3) {
    fprintf(stderr, "USAGE:./assign2 data.txt output.txt\n");
    exit(1);
  }

  pthread_t tid[3]; // three threads
  ThreadParams params;


  // Initialization
  initializeData(&params);
  strcpy(params.inputFile, argv[1]); 
  strcpy(params.outputFile, argv[2]); 

  // Create Threads
  pthread_create(&(tid[0]), &attr, &ThreadA, (void*)(&params));
  pthread_create(&(tid[1]), &attr, &ThreadB, (void*)(&params));
  pthread_create(&(tid[2]), &attr, &ThreadC, (void*)(&params));


  // Wait on threads to finish
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  pthread_join(tid[2], NULL);

  return 0;
}

void initializeData(ThreadParams *params) {
  // Initialize Sempahores
  if(sem_init(&(params->sem_A), 0, 1) != 0) { // Set up Sem for thread A
    perror("error for init threa A");
    exit(1);
  }
  if(sem_init(&(params->sem_B), 0, 0) != 0) { // Set up Sem for thread B
    perror("error for init threa B");
    exit(1);
  }
  if(sem_init(&(params->sem_C), 0, 0) != 0) { // Set up Sem for thread C
    perror("error for init threa C");
    exit(1);
  } 
  
  // initialize pipe
  int result = pipe(params->pipeFile);
  if (result < 0){
    perror("pipe error");
    exit(1);
  }

  // Initialize thread attributes 
  pthread_attr_init(&attr);

  return;
}

void* ThreadA(void *params) {

  sem_wait(&((ThreadParams*)params)->sem_A);
  
  char buf[1000];
  FILE* file;
  char* input = ((ThreadParams*)params)->inputFile;
  int* pipe = ((ThreadParams*)params)->pipeFile;

  if ((file = fopen(input, "r")) == NULL) {
    printf("Error! opening file");
    // Program exits if file pointer returns NULL.
    exit(1);
  }

  // reads all lines from data.txt
  char* str_read;
  while ((str_read = fgets(buf, sizeof(buf), file)) != NULL) {
    int write_result = write(pipe[1], buf, strlen(str_read)); 
    if (write_result < 0) {
      printf("error writing to pipe.");
      exit(1);
    }
  }
  
  if (pipe[1] == -1) {
    perror("pipe error");
  } 

  for (int i = 0; i < 5; i++) { 
    sum = 2 * sum; 
  }

  printf("Thread A: sum = %d\n", sum);

  sem_post(&((ThreadParams*)params)->sem_B);

  return NULL;
}

void* ThreadB(void *params) {
  sem_wait(&((ThreadParams*)params)->sem_B);

  int* pipe = ((ThreadParams*)params)->pipeFile;
  shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT|O_RDWR, 0666);
  int ftrunc_return = ftruncate(shm_fd, SHARED_MEM_SIZE);
  if (ftrunc_return < 0)  {
    printf("error occured");
    exit(1);
  }
  void* shm_ptr = mmap(0, SHARED_MEM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
  // the contents of the pipe will be copied to this buffer
  char buf[SHARED_MEM_SIZE];

  int result = read(pipe[0], buf, SHARED_MEM_SIZE);
  if (result == -1)  {
    printf("Error reading from pipe");
    exit(1);
  }

  if (sprintf(shm_ptr, "%s", buf) < 0) {
    printf("Error writing to shared memory");
    exit(1);
  }

  for (int i = 0; i < 3; i++) {
    sum = sum * 3;
  }

  printf("Thread B: sum = %d\n", sum);
  sem_post(&((ThreadParams*)params)->sem_C);

  return NULL;
}

void* ThreadC(void *params) {
  sem_wait(&((ThreadParams*)params)->sem_C);

  FILE* output_file_ptr;
  shm_fd = shm_open(SHARED_MEM_NAME, O_RDONLY, 0666);
  void* shm_ptr = mmap(0, SHARED_MEM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
  char shm_contents[SHARED_MEM_SIZE];
  strcpy(shm_contents, (char*)shm_ptr);


  remove(((ThreadParams*)params)->outputFile);
  if ((output_file_ptr = fopen(((ThreadParams*)params)->outputFile, "a")) == NULL) {
    printf("Error opening output file");
    exit(1);
  }
  
  // split the string into lines and iterate through each line checking wether the end_header flag has passed
  char* line = strtok(shm_contents, "\n");
  int is_header = 1; 
  while (line != NULL) {
    if (is_header == 0) {
      fprintf(output_file_ptr, "%s\n", line);
      printf("%s\n", line);
    }
    if (strcmp(line, "end_header") == 0) {
      is_header = 0;
    }
    line = strtok(NULL, "\n");
  }

  shm_unlink(SHARED_MEM_NAME);

  // Calculate sum variable
  for (int i = 0; i < 4; i++) {
    sum = sum - 5;
  }

  printf("Thread C: Final sum = %d", sum);

  return NULL;
}


