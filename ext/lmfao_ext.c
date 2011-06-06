#include <ruby.h>
#include <pthread.h>
#include "lmfao.h"

static VALUE mLMFAO_call_nogvl(void *data);
void *lmfao_callback(void *data);

/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Functions related to the global callback queue.
 * * * * * * * * * * * * * * * * * * * * * * * * */

/*
   Three globals to allow for Ruby/C-thread communication:

   - mutex & condition to synchronize access to callback_queue
   - callback_queue to store actual callback data in

   Be careful with the functions that manipulate the callback
   queue; they must do so in the protection of a mutex.
*/
pthread_mutex_t g_callback_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_callback_cond   = PTHREAD_COND_INITIALIZER;
callback_t *g_callback_queue     = NULL;

/*
   Use this function to add a callback node onto the global
   callback queue.

   Do note that we are adding items to the front of the linked
   list, and as such events will always be handled by most recent
   first. To remedy this, add to the end of the queue instead.
*/
void g_callback_queue_push(callback_t *callback)
{
  callback->next   = g_callback_queue;
  g_callback_queue = callback;
}

/*
   Use this function to pop off a callback node from the
   global callback queue. Returns NULL if queue is empty.
*/
static callback_t *g_callback_queue_pop(void)
{
  callback_t *callback = g_callback_queue;
  if (callback)
  {
    g_callback_queue = callback->next;
  }
  return callback;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Call LMFAO with the given argument.
 *
 * @param [Object] data given to LMFAO
 * @return [Object] whatever is returned by LMFAO
 */
static VALUE mLMFAO_call(VALUE self, VALUE data)
{
  rb_need_block();

  VALUE args = rb_ary_new();
  rb_ary_push(args, rb_block_proc());
  rb_ary_push(args, data);

  VALUE result = rb_thread_blocking_region(mLMFAO_call_nogvl, (void *) args, NULL, NULL);
  return result;
}

static VALUE mLMFAO_call_nogvl(void *data)
{
  return (VALUE) lmfao_call(lmfao_callback, data);
}

/*
  This is our user-defined C callback, it gets called by the C library.

  We need to:
    1. acquire lock around global callback queue
    2. put (our) callback data in the global callback queue
    3. signal and unlock the global callback queue
    4. wait until the callback has been handled
    5. clean up after ourselves
    6. return the result to our caller
*/
void *lmfao_callback(void *data)
{
  return (void *) Qfalse;
}

/**
 * Ruby bindings for LMFAO C library.
 */
void Init_lmfao_ext()
{
  VALUE mLMFAO = rb_define_class("LMFAO", rb_cObject);
  rb_define_singleton_method(mLMFAO, "call", mLMFAO_call, 1);
}
