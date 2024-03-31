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

  //TODO: add your code


  // Wait on threads to finish
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  pthread_join(tid[2], NULL);


  //TODO: add your code

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

  // Initialize thread attributes 
  pthread_attr_init(&attr);
  //TODO: add your code

  return;
}

void* ThreadA(void *params) {
  //TODO: add your code
  sem_wait(&((ThreadParams*)params)->sem_A);

  printf("Thread A: sum = %d\n", sum);

  sem_post(&((ThreadParams*)params)->sem_B);
}

void* ThreadB(void *params) {
  sem_wait(&((ThreadParams*)params)->sem_B);

  //TODO: change to read from pipe
  char temp_str[1024];

  shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT|O_RDWR, 0666);
  ftruncate(shm_fd, SHARED_MEM_SIZE);
  void* shm_ptr = mmap(0, SHARED_MEM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);

  read_file(((ThreadParams*)params)->inputFile, shm_ptr);

  for (int i = 0; i < 3; i++) {
    sum = sum * 3;
  }

  printf("Thread B: sum = %d\n", sum);
  sem_post(&((ThreadParams*)params)->sem_C);
}

void* ThreadC(void *params) {
  sem_wait(&((ThreadParams*)params)->sem_C);
  FILE* output_file_ptr;
  shm_fd = shm_open(SHARED_MEM_NAME, O_RDONLY, 0666);
  ftruncate(shm_fd, SHARED_MEM_SIZE);
  void* shm_ptr = mmap(0, SHARED_MEM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
  char shm_contents[SHARED_MEM_SIZE];
  strcpy(shm_contents, (char*)shm_ptr);

  remove(((ThreadParams*)params)->outputFile);
  if ((output_file_ptr = fopen(((ThreadParams*)params)->outputFile, "a")) == NULL) {
    printf("Error opening output file");
    exit(1);
  }

  char* line = strtok(shm_contents, "\n");
  int is_header = 1; 
  while (line != NULL) {
    if (is_header == 0) {
      fprintf(output_file_ptr, "%s\n", line);
    }
    if (strcmp(line, "end_header") == 0) {
      is_header = 0;
    }
    line = strtok(NULL, "\n");
  }

  // Calculate sum variable
  for (int i = 0; i < 4; i++) {
    sum = sum - 5;
  }

  printf("Thread C: Final sum = %d\n", sum);
}

// temporary read function
int read_file(char file_name[], void* shm_ptr) {
  char c[1000];
  int sig;
  FILE* fptr;
  char check[1000]="end_header";
  if ((fptr = fopen(file_name, "r")) == NULL) {
    printf("Error! opening file");
    // Program exits if file pointer returns NULL.
    exit(1);
  }

  // reads text until newline is encountered
  sig=0;
  while(fgets(c, sizeof(c), fptr) != NULL) {
    if (sprintf(shm_ptr, "%s", c) < 0) {
      printf("Error writing to shared memory");
      exit(1);
    }
    shm_ptr += strlen(c); // increase the pointer of address for writing the next line
  }
  fclose(fptr);
  return 0;
}
