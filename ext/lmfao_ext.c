#include <ruby.h>
#include <pthread.h>
#include "lmfao.h"

static VALUE mLMFAO_call_nogvl(void *data);
void *lmfao_callback(void *data);

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