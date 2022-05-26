/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);
  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }  
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  
  ASSERT (sema != NULL);
  //////////////////////
  struct thread *t=NULL; // create a struct to hold the thread for comparison
  //////////////////////
  old_level = intr_disable ();
  if (!list_empty (&sema->waiters))
  {
    /////////////////////////////////////////////////////
    //get maximum of the waiters
    struct list_elem *s = list_min(&sema->waiters, &list_less_comp, NULL);
    list_remove(s);//remove the element from the list
     t = list_entry(s, struct thread, elem);//get the thread
    thread_unblock(t);// unblock it
     ////////////////////////////////////////////////////////
  }
  sema->value++;
  intr_set_level (old_level);

  //////////////////////////////////////////////
  //if there were threads in the waiting and their maximum's priority is higher
  /*if(t != NULL&& t->priority > thread_current()->priority)                                                          
      thread_yield();// yield the CPU*/
    ///////////////////////////////////////////////
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);
  lock->holder = NULL;
  /////////////////////////////////
  //lock->max_prio = PRI_MIN;// initial value for the priority is lowest 0
  /////////////////////////////////
  sema_init (&lock->semaphore, 1);
}

/*
  list_less_func used to compare between the maximum priority of 2 given
  locks used in lock_release() to get the next priority value to be assigned 
  to the priority of the thread holding a number of locks */
bool 
list_less_complocks(const struct list_elem* a, const struct list_elem* b,
               void* aux UNUSED)
{
  const int a_member = (list_entry(a, struct lock, elem_thread)->max_prio);
  const int b_member = (list_entry(b, struct lock, elem_thread)->max_prio);
  
  return a_member < b_member;
}
/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  //////////////////////////////////////////////
  //if the holder has been determined and we are not working with
  // mlfqs scheduler
  enum intr_level old_level = intr_disable();
  if(!thread_mlfqs && lock->holder != NULL)
  {
      // then the entering thread will wait on this lock
      thread_current()->waiting_lock = lock;
       //disable the interrupt to start the chaining process
    
      //start a pointer to the current lock
      struct lock *wait=lock;
      //start with a pointer to the current thread(the waiting one)
      struct thread *cur= thread_current();
      // while the maximum priority of the current lock is lower
      // than the thread that is waiting on it and we have not 
      // reached the end of the chain
       while(wait!=NULL && wait->max_prio < cur->priority)
       {
         //donate your priority to this lock
         wait->max_prio=cur->priority;
         //and if the holder's priority is lower
         if(cur->priority>wait->holder->priority){
           // donate your priority to it as well
           wait->holder->priority=cur->priority;
         }
        // move the pointer to the next lock that the holder of 
        // the current lock is waiting on in the chain so that
        // we can donate this current priority to it
         wait=wait->holder->waiting_lock;
       }
     
    
 //////////////////////////////////////
  }

 /* here the waiters will enter the queue */
  sema_down (&lock->semaphore);
  /*solution*/
  /* here the holder is determined */
  lock->holder = thread_current ();
  intr_set_level(old_level);
  //if we are not working with the mlfqs scheduler 
  if(!thread_mlfqs){
  thread_current()->waiting_lock = NULL;
  //the maximum priority of the lock is by default its holder's priority 
  lock->max_prio = lock->holder->priority;
  // push this lock into the holder's list of locks it its holding
  list_push_back(&lock->holder->locks,  &lock->elem_thread);
  }
  ////////////////////////////////////////////////////////

}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));
  //if we are not using the mlfqs scheduler
  if(!thread_mlfqs){
  list_remove(&lock->elem_thread);//remove this lock from the holder's list
  // if the priority of the holder bigger than its lock's priority
  if(lock->holder->priority >=lock->max_prio)
  {
    //if the locks is still holding onto other lock then it is time to choose 
    // a new priority to be donated to it
    if(!list_empty(&lock->holder->locks) )
    {
      // get the maximum of the max priorities inside the holder's list
      int prio= list_entry(list_max(&lock->holder->locks, &list_less_complocks,NULL)
                                        , struct lock, elem_thread)->max_prio;
      // if this prioirty is higher than the orignial priority of the holder
      if(prio> lock->holder->base_priority)
         lock->holder->priority =prio;// donate it
      else  
      //else we are going to work with the holder's priority
         lock->holder->priority = lock->holder->base_priority;
    }
    else
        //if the thread is not holding any more locks reset its priority back
         lock->holder->priority = lock->holder->base_priority;
  }
  else
      //if the holder's priority is smaller we reset it back
       lock->holder->priority = lock->holder->base_priority;
  }
  // after releasing the lock reset the holder to null
  lock->holder = NULL;
  // choose the next from the waiting list
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/*
* this function is used for in sorting the semaphore 
* according to there max priority  that wait on condition
* so when pop at max priority 
* (lsit less function from the CS350)
*/
bool
list_less_compcond(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{ 
  struct semaphore_elem *sema_a = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *sema_b = list_entry(b, struct semaphore_elem, elem);
  struct thread * t = list_entry(list_min(&sema_a->semaphore.waiters, &list_less_comp, NULL), struct thread, elem);
  struct thread * k= list_entry(list_min(&sema_b->semaphore.waiters,&list_less_comp, NULL), struct thread, elem);

  return t->priority >  k->priority;
  
}


/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  if (!list_empty (&cond->waiters)){
    /////////////////////////////////////////////////////////////////////////////////
      /* pick the next list element highest semaphore element
         who has the highest priority thread in their waiting list */
      struct list_elem * e = list_min(&cond->waiters, &list_less_compcond,NULL);
      // remove the element from the list
      list_remove(e);
      //get the semaphore element from the list element
      struct semaphore_elem* s = list_entry(e,struct semaphore_elem, elem);
      sema_up(&s->semaphore);//release the thread on this semaphore
      /////////////////////////////////////////////////////////////////////////////
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

