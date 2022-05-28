#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
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
void close_file(int);
int get_int(int* esp);
char* get_char_ptr(int* esp);
void* get_void_ptr(int* esp);
bool create(const char* file, unsigned initial_size);
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
struct files* get_file(struct list, int);

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
  //printf("system call \n");
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
      // printf("exit******************************");
      validate_pointer(((int*)f->esp+1));
      int status = *((int*)f->esp+1);
      // process_exit();
      exit(status);
      break;
    }
    case SYS_EXEC:
    {
        //printf("excu****************************** \n");
        validate_pointer(((int*)f->esp+1));
        //printf("char:%s \n",(char*)*((int*)f->esp+1));
        //  const char* sent= (char*)f->esp+1;
        //  printf("char:%s \n",sent);
        f->eax = process_execute( (char*)*((int*)f->esp+1));
        break;
    }
    case SYS_WAIT:
      f->eax = wait_wrapper(f->esp);
      break;

    case SYS_OPEN:
    {
      validate_pointer(((int*)f->esp+1));
      char* name = *(((int*)f->esp+1));
      lock_acquire(&lock);
      struct file *ptr = filesys_open(name);
      lock_release(&lock);
      if(ptr == NULL)
      {
        f->eax = -1;
        break;
      }
      else
      {
        struct files* file_struct = (struct files*)malloc(sizeof(struct files));
        file_struct-> fd = thread_current()->fds;
        file_struct-> file = ptr;
        list_push_back(&thread_current()->files,&file_struct->elem);
        thread_current()->fds++;
        break;
      }
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
        exit(-1);
      }
      // writing to STDOUT
      else if(fd == 1)
      {
        putbuf(buffer, size);
        f->eax = (int)size;
      }
      else
      {
        struct files* ptr = get_file(thread_current()->files, fd);
        if(ptr == NULL)
        {
          f->eax = -1;
        }
        else
        {
          lock_acquire(&lock);
          f->eax = file_write(ptr->file, buffer, size);
          lock_release(&lock);
        }
      }
      break;
    }
    case SYS_READ:
    {
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      validate_pointer(((int*)f->esp+2));
      void * buffer = (void*)(*((int*)f->esp+2));
      validate_pointer(((int*)f->esp+3));
      unsigned size = *((unsigned*)f->esp+3);
      // reading from STDOUT
      if(fd == 1)
      {
        exit(-1);
      }
      // reading from STDIN
      else if(fd == 0)
      {    
        buffer = input_getc();
        f->eax = (int)size;
      }
      else
      {
        struct files* ptr = get_file(thread_current()->files, fd);
        if(ptr == NULL)
        {
          f->eax = -1;
        }
        else
        {
          lock_acquire(&lock);
          f->eax = file_read(ptr->file, buffer, size);
          lock_release(&lock);
        }
      }
      break;
    }

    case SYS_FILESIZE:
    {
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      struct files* ptr = get_file(thread_current()->files, fd);
      if(ptr == NULL)
      {
        f->eax = -1;
      }
      else
      {
        lock_acquire(&lock);
        f->eax = file_length(ptr->file);
        lock_release(&lock);
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

    case SYS_CLOSE:
    {
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      close_file(fd);
    }

  }
}

void validate_pointer(void * p)
{
  // check if the pointer is null
  if(p == NULL)
  {
    exit(-1);
  }
  // check if the pointer within user program space
  else if(! is_user_vaddr(p))
  {
    exit(-1);
  }
  // check if the pointer within the process's page
  // this function is used instead of lookup_page as it is static
  else if(pagedir_get_page(thread_current()->pagedir, p) == NULL)
  {
    exit(-1);
  }
}


bool 
create(const char* file, unsigned initial_size){
   // printf("got herer 1\n");
  lock_acquire(&lock);
   // printf("got herer 2,%s, %d \n",file ,initial_size);
  bool is_created = filesys_create(file, initial_size);
   // printf("got herer 3\n");
  lock_release(&lock);
 //printf("got herer 4\n");
  return is_created;
}

bool 
create_wrapper(void* esp){
  validate_pointer((int*)esp+1);
  const char* file = get_char_ptr((int*)esp+1);
  validate_pointer((int*)esp +2);
  unsigned size = get_int((int*)esp+2);
  return create(file,(unsigned)size);
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
       unsigned position = file_tell(file->file);
       lock_release(&lock);
       return position;
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


struct files* get_file(struct list list, int fd)
{
  struct list_elem* iter = list_begin(&list);
  struct files *temp = NULL;
  while(iter != list_end(&list))
  {
    struct files* t = list_entry(iter,struct files,  elem);
    if(t->fd == fd)
    {
      temp = t;
      break;
    }
    iter = list_next(iter);
  }
  return temp;
}

void close_file(int fd)
{
  struct files *f = get_file(thread_current()->files, fd);
  list_remove(&f->elem);
  lock_acquire(&lock);
  file_close(f->file);
  lock_release(&lock); 
}

void exit(int status)
{
  thread_current()->exit_status = status;
  struct list_elem* iter = list_begin(&thread_current()->files);
  while(iter != list_end(&thread_current()->files))
  {
    struct files* t = list_entry(iter,struct files,  elem);
    lock_acquire(&lock);
    file_close(t->file);
    lock_release(&lock);    
    iter = list_next(iter);
  }
  printf ("%s: exit(%d)\n",thread_current()->name, thread_current()->exit_status);
  thread_exit();
}
