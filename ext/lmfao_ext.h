#ifndef LMFAO_EXT_H_CMPA72FF
#define LMFAO_EXT_H_CMPA72FF

#define bool int
#define true 1
#define false 0
#define DEBUG(str) printf("[%d] (%s) " str "\n", __LINE__, __FUNCTION__)

typedef struct callback_t callback_t;
struct callback_t {
  /*
     Each callback needs to store its’ data somewhere; this
     is how we later on access that data from our Ruby thread.

     We also use this for our return value from the Ruby handler.
  */
  void *data;

  /*
     Once we’ve dispatched our callback data to Ruby, we must
     wait for a reply before we continue. These two are used
     for that purpose.
  */
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

  /*
     Even though we use the condition variable above to wait,
     we might still be woken up (spurious wakeups). This bool
     serves as a final check that tells us if we can continue.
  */
  bool handled;

  /*
     We use this to implement a linked list of callback data.
     This allows multiple callbacks to happen simultaneously
     without them having to wait for each other.
  */
  callback_t *next;
};

#endif /* end of include guard: LMFAO_EXT_H_CMPA72FF */
