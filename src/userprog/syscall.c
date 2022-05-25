#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "pagedir.h"
#include "syscall-nr.h"
#include "process.h"
#include "list.h"

static void syscall_handler (struct intr_frame *);
void validate_pointer(void* p);
void exit(int);
struct lock lock;

struct files{
    int fd;
    struct list_elem elem;
    struct file* file;
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // check if the esp pointer is valid within user program space
  validate_pointer(f->esp);
  switch(*(int*)f->esp){

    case SYS_HALT:
    {
      shutdown_power_off();
      break;
    }

    case SYS_EXIT:
    {
      validate_pointer(((int*)f->esp+1));
      int status = *((int*)f->esp+1);
      exit(status);
      break;
    }
    case SYS_EXEC:
    {
        validate_pointer(((int*)f->esp+1));
        // process_execute((char*)*((int*)f->esp+1));
        break;
    }

    case SYS_WRITE:
    {
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      validate_pointer(((int*)f->esp+2));
      void * buffer = (void*)(*((int*)f->esp+2));
      validate_pointer(((int*)f->esp+3));
      unsigned size = *((unsigned*)f->esp+3);
      // writing to STDIN
      if(fd == 0)
      {
        // process_exit(-1);
      }
      // writing to STDOUT
      else if(fd == 1)
      {
        putbuf(buffer, size);
        f->eax = (int)size;
      }
      else
      {
        // file_write();
      }
      break;
    }

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

void exit(int status)
{
  thread_current()->exit_status = status;
  printf ("%s: exit(%d)\n",thread_current()->name, thread_current()->exit_status);
  thread_exit();
}