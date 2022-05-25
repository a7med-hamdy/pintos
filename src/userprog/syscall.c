#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "syscall-nr.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void validate_pointer(void* p);



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  // check if the esp pointer is valid within user program space
  validate_pointer(f->esp);
  switch(*(int*)f->esp){

    case SYS_EXEC:
    {
        validate_pointer(((int*)f->esp+1));
        process_execute((char*)*((int*)f->esp+1));
        break;
    }

    // sys write is done first to allow printf
    // case SYS_WRITE:
    // {
    //   validate_pointer(((int*)f->esp+1));
    //   int fd = *(((int*)f->esp+1));
    //   validate_pointer(((int*)f->esp+2));
    //   void * buffer = (void*)(*((int*)f->esp+2));
    //   validate_pointer(((int*)f->esp+3));
    //   unsigned size = *((unsigned*)f->esp+3);
    //   if(fd == 1)
    //   {
    //     f->eax = putbuf(buffer, size);
    //   }
    //   break;
    // }

  }

  thread_exit ();
}

void validate_pointer(void * p)
{
  // check if the pointer is null
  if(p == NULL)
  {
    process_exit();
  }
  // check if the pointer within user program space
  else if(! is_user_vaddr(p))
  {
    process_exit();
  }
  // check if the pointer within the process's page
  // this function is used instead of lookup_page as it is static
  else if(pagedir_get_page(thread_current()->pagedir, p) == NULL)
  {
    process_exit();
  }
}