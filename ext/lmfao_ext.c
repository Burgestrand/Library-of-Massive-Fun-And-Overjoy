#include <ruby.h>
#include <pthread.h>
#include "lmfao.h"
#include "lmfao_ext.h"

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

/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Functions related to LMFAO Ruby API
 * * * * * * * * * * * * * * * * * * * * * * * * */

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

  /* this is just so we can call LMFAO function and still allow other ruby threads to run
   * while we wait for a result*/
  return rb_thread_blocking_region(mLMFAO_call_nogvl, (void *) args, NULL, NULL);
}

static VALUE mLMFAO_call_nogvl(void *data)
{
  return (VALUE) lmfao_call(lmfao_callback, data);
}

/*
  This is our user-defined C callback, it gets called by the C library.

  We need to:
    1. Create a callback structure, put our parameters in it
    2. Push the callback node onto the global callback queue
    3. Wait for the callback to be handled
    4. Return the return value
*/
void *lmfao_callback(void *data)
{
  return (void *) Qfalse;
}

/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Our special Ruby event-listening thread functions
 * * * * * * * * * * * * * * * * * * * * * * * * */

/*
   Executed for each callback notification; what we receive
   are the callback parameters. The job of this method is to:

   1. Convert callback parameters into Ruby values
   2. Call the appropriate callback with said parameters
   3. Convert the Ruby return value into a C value
   4. Hand over the C value to the C callback
*/
static VALUE LMFAO_handle_callback(void *cb)
{
  return Qnil;
}

typedef struct callback_waiting_t callback_waiting_t;
struct callback_waiting_t {
  callback_t *callback;
  bool abort;
};

/*
   Wait for global callback queue to contain something.

   This function is called while not holding the GIL, so it’s safe
   to do long waits in here; other threads will still run.

   The job of this function is merely to wait until the callback queue
   contains something. Once that happens, we remove the item from the
   queue and return it to our caller through waiting->callback.
*/
static VALUE wait_for_callback_signal(void *w)
{
  callback_waiting_t *waiting = (callback_waiting_t*) w;

  pthread_mutex_lock(&g_callback_mutex);

  // abort signal is used when ruby wants us to stop waiting
  while (waiting->abort == false && (waiting->callback = g_callback_queue_pop()) == NULL)
  {
    pthread_cond_wait(&g_callback_cond, &g_callback_mutex);
  }

  pthread_mutex_unlock(&g_callback_mutex);

  return Qnil;
}

/*
   Stop waiting for callback notification. This function
   is invoked by Ruby when she wants us to exit.

   As `wait_for_callback_signal` function is executed without holding
   the GIL, it can potentially take forever. However, when somebody wants
   to quit the program (for example if somebody presses CTRL-C to exit)
   we must tell Ruby how to wake up the `wait_for_callback_signal` function
   so Ruby can exit properly.
*/
static void stop_waiting_for_callback_signal(void *w)
{
  callback_waiting_t *waiting = (callback_waiting_t*) w;

  pthread_mutex_lock(&g_callback_mutex);
  waiting->abort = true;
  pthread_cond_signal(&g_callback_cond);
  pthread_mutex_unlock(&g_callback_mutex);
}

/*
  This thread loops continously, waiting for callbacks to happen in C.
  Once they do, it will handle the callback in a new thread.

  The reason we use a thread for handling the callback is that the handler
  itself might fire off more callbacks, that might need to be handled before
  the handler returns. We can’t do that if the event thread is busy handling
  the first callback.

  Continously, we need to:
    1. Unlock the Ruby GVL (allowing other threads to run while we wait)
    2. Wait for a callback to fire
    3. Spawn a new ruby thread to handle the callback
*/
static VALUE LMFAO_event_thread(void *unused)
{
  callback_waiting_t waiting = {
    .callback = NULL,
    .abort    = false
  };

  while (waiting.abort == false)
  {
    // release the GIL while waiting for a callback notification
    rb_thread_blocking_region(wait_for_callback_signal, &waiting, stop_waiting_for_callback_signal, &waiting);

    // if ruby wants us to abort, this will be NULL
    if (waiting.callback)
    {
      rb_thread_create(LMFAO_handle_callback, (void *) waiting.callback);
    }
  }

  return Qnil;
}

 /* * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Ruby bindings for LMFAO C library.
 *
 * Requiring the LMFAO C library will also create the event thread
 * at the same time. In reality you can wait with this until you
 * need to have it running.
 */
void Init_lmfao_ext()
{
  VALUE mLMFAO = rb_define_class("LMFAO", rb_cObject);
  rb_define_singleton_method(mLMFAO, "call", mLMFAO_call, 1);
  rb_thread_create(LMFAO_event_thread, NULL);
}
