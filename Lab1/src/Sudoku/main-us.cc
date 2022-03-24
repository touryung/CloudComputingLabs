#include <assert.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <vector>

#include "pthread.h"
#include "sudoku.h"

using namespace std;

pthread_mutex_t readWriteLock = PTHREAD_MUTEX_INITIALIZER;  // 读写锁
pthread_mutex_t outputLock = PTHREAD_MUTEX_INITIALIZER;     // 输出锁
pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;       // 文件锁
pthread_cond_t outCond = PTHREAD_COND_INITIALIZER;  // 输出条件变量

sem_t queueEmpty;  // 任务队列空的信号量
sem_t queueFull;   // 任务队列满的信号量

int CPU_NUM = sysconf(_SC_NPROCESSORS_ONLN);  // CPU 数目
int n = CPU_NUM - 1;

FILE* fp;           // 读取文件的指针
char* currentFile;  // 当前文件

#define BUFSIZE 1000
char puzzle[BUFSIZE][128];
int producerNum = 0;
int consumerNum = 0;
bool sysend = false;
int total = 0;
int totalSolved = 0;
int outOrder = 0;

int64_t now() {  // 计算现在的时间
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char* argv[]) {
  sem_init(&queueEmpty, 0, BUFSIZE);
  sem_init(&queueFull, 0, 0);

  double totalSec = 0.0;

  pthread_t p[n];
  for (int i = 0; i < n; i++) pthread_create(&p[i], NULL, solver, NULL);

  // 读取文件
  char filename[20];
  while (1) {
    scanf("%s", filename);

    int64_t start = now();
    pthread_mutex_lock(&fileLock);
    while (currentFile != NULL) {
    }
    currentFile = filename;
    pthread_mutex_unlock(&fileLock);

    pthread_t t;
    pthread_create(&t, NULL, reader, (void*)filename);
    pthread_join(t, NULL);
    int64_t end = now();

    double sec = (end - start) / 1000000.0;
    totalSec += sec;
  }

  // 收回生产者
  for (int i = 0; i < n; i++) pthread_join(p[i], NULL);
  printf("%f sec %f ms each %d\n", totalSec, 1000 * totalSec / total,
         totalSolved);
  return 0;
}