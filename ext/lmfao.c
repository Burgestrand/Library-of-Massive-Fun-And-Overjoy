#include "lmfao.h"
#include <pthread.h>

void *lmfao_call(LMFAO_CALLBACK fn, void *data)
{
  void *result = NULL;
  
  pthread_t workhorse;
  pthread_create(&workhorse, NULL, fn, data);
  pthread_join(workhorse, &result);
  
  return result;
}