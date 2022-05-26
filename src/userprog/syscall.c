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
#include <inttypes.h>

static void syscall_handler (struct intr_frame *);
void validate_pointer(void* p);
int get_int(int* esp);
char* get_char_ptr(int* esp);
void* get_void_ptr(int* esp);
bool create_wrapper(void* esp);
bool remove_wrapper(void* esp);
int wait_wrapper(void* esp);
unsigned tell_wrapper(void* esp);
void seek_wrapper(void* esp);


struct lock lock;
void exit(int);

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
  // printf("*******************system call intialized***********************\n");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //debug_backtrace_all();
  // check if the esp pointer is valid within user program space
  validate_pointer(f->esp);
  switch(*(int*)f->esp){

    case SYS_HALT:
    {
       //debug_backtrace_all();
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
    case SYS_WAIT:
      f->eax = wait_wrapper(f->esp);
      break;

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
    case SYS_CREATE: 
      f->eax = create_wrapper(f->esp);
      break;
    case SYS_REMOVE:
      f->eax = remove_wrapper(f->esp);
      break;
    case SYS_SEEK:
      seek_wrapper(f->esp);
      break;
    case SYS_TELL:
      f->eax = tell_wrapper(f->esp);
      break;





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


bool 
create(const char* file, unsigned initial_size){
  lock_acquire(&lock);
  bool is_created = filesys_create(file, initial_size);
  lock_release(&lock);
  return is_created;
}

bool 
create_wrapper(void* esp){
  validate_pointer((int*)esp+1);
  char* file = get_char_ptr((int*)esp+1);
  validate_pointer((int*)esp +2);
  unsigned size = get_int((int*)esp+2);
  return create(file, size);
}


void 
seek(int fd, unsigned position){
  //get current thread list of files
  struct list files = thread_current()->files;
  struct list_elem* iter = list_begin(&files);
  //iterate through files list and match fd
  while(iter != list_end(&files)){
    struct files* file = list_entry(iter, struct files, elem);
    if(file->fd == fd){
       lock_acquire(&lock);
       file_seek(file->file, position);
       lock_release(&lock);
       return;
    }
    iter = list_next(iter);
  }
}

void 
seek_wrapper(void* esp){
  validate_pointer((int*)esp+1);
  int fd = get_int((int*)esp+1);
  validate_pointer((int*)esp+2);
  unsigned position =  get_int((int*)esp+2);
  seek(fd, position);
}

unsigned 
tell(int fd){
  //get current thread list of files
  struct list files = thread_current()->files;
  struct list_elem* iter = list_begin(&files);
  //iterate through files list and match fd
  while(iter != list_end(&files)){
    struct files* file = list_entry(iter, struct files, elem);
    if(file->fd == fd){
       lock_acquire(&lock);
       unsigned position = tell(fd);
       lock_release(&lock);
       return fd;
    }
    iter = list_next(iter);
  }
}

unsigned 
tell_wrapper(void* esp){
  validate_pointer((int*)esp + 1);
  int fd = get_int((int*)esp + 1);
  return tell(fd);
}


bool 
remove(const char* file){
  lock_acquire(&lock);
  bool is_removed = filesys_remove(file);
  lock_release(&lock);
  return is_removed;
}


bool 
remove_wrapper(void* esp){
  validate_pointer((int*) esp +1);
  char* file = get_char_ptr(((int*) esp +1));
  return remove(file);
}


int
 wait(tid_t pid){
  return process_wait(pid);
}
int 
wait_wrapper(void* esp){
  validate_pointer((int*)esp +1);
  tid_t pid = get_int((int*)esp +1);
  return wait(pid);

}


int 
get_int(int* esp){
  return *esp;
}


char* get_char_ptr(int* esp){
  return (char*) *esp;
}

void* get_void_ptr(int* esp){
  return (void*) *esp;
}


void exit(int status)
{
  thread_current()->exit_status = status;
  printf ("%s: exit(%d)\n",thread_current()->name, thread_current()->exit_status);
  thread_exit();
}
