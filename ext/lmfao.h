#ifndef LMFAO_H_Q4TUWT6T
#define LMFAO_H_Q4TUWT6T

  typedef void *(LMFAO_CALLBACK)(void *);
  
  /**
   * lmfao_call is the main workhorse of LMFAO.
   *
   * notes:
   *   LMFAO will join the worker thread, so release your GVL before calling
   *
   * parameters:
   *   fn    — user-supplied callback to call in a separate (non-ruby) thread
   *   data  — data to give to the user-supplied function
   *
   * returns whatever the fn returns
   */
  void *lmfao_call(LMFAO_CALLBACK fn, void *data);

#endif /* end of include guard: LMFAO_H_Q4TUWT6T */
