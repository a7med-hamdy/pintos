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
struct files* get_file(struct list*, int);
void exit(int);

// lock to synchronize files' operations
struct lock lock;

// a struct to hold file pointer with the corresponding
// descriptor in the thread's list
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
  // validate the esp pointer (top of the stack pointer)
  validate_pointer(f->esp);
  switch(*(int*)f->esp){
    //halt system call
    case SYS_HALT:
    {
      shutdown_power_off();
      break;
    }
    // exit system call
    case SYS_EXIT:
    {
      //pull exit status from the stack and validate it
      validate_pointer(((int*)f->esp+1));
      int status = *((int*)f->esp+1);
      // preform the exit operation
      exit(status);
      break;
    }
    // exec system call
    case SYS_EXEC:
    {
      //pull the command from the stack and validate it
      char* cmd = get_char_ptr(((int*)f->esp +1));
      validate_pointer(cmd);
      // execute the command
      f->eax = process_execute(cmd);
      break;
    }
    case SYS_WAIT:
      f->eax = wait_wrapper(f->esp);
      break;
    // open system call
    case SYS_OPEN:
    {
      // pull the file's name from the stack and validate it
      char* name = *(((int*)f->esp+1));
      validate_pointer(name);
      lock_acquire(&lock);
      struct file *ptr = filesys_open(name);
      lock_release(&lock);
      // if file not found return -1
      if(ptr == NULL)
      {
        f->eax = -1;
        break;
      }
      else
      {
        //add the file to the thread's list and assign it a descriptor
        struct files* file_struct = (struct files*)malloc(sizeof(struct files));
        file_struct-> fd = thread_current()->fds;
        file_struct-> file = ptr;
        list_push_back(&thread_current()->files,&file_struct->elem);
        f->eax = thread_current()->fds;
        thread_current()->fds++;
        break;
      }
     
    }
    // write system call
    case SYS_WRITE:
    {
      // pull arguement from the stack and validate them
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      validate_pointer((void*)(*((int*)f->esp+2)));
      void * buffer = (void*)(*((int*)f->esp+2));
      validate_pointer(((int*)f->esp+3));
      unsigned size = *((unsigned*)f->esp+3);
      // writing to STDIN (illegal)
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
        struct files* ptr = get_file(&thread_current()->files, fd);
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
    //read system call
    case SYS_READ:
    {
      // pull arguements from the stack and validate them
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      validate_pointer((void*)(*((int*)f->esp+2)));
      void * buffer = (void*)(*((int*)f->esp+2));
      validate_pointer(((int*)f->esp+3));
      unsigned size = *((unsigned*)f->esp+3);
      // reading from STDOUT (illegal)
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
      // reading from file
      else
      {
        // get the file with the given descriptor
        struct files* ptr = get_file(&thread_current()->files, fd);
        // if not found return -1
        if(ptr == NULL)
        {
          f->eax = -1;
        }
        // preform the read operation
        else
        {
          lock_acquire(&lock);
          f->eax = file_read(ptr->file, buffer, size);
          lock_release(&lock);
        }
      }
      break;
    }
    // file size system call
    case SYS_FILESIZE:
    {
      // validate the pointer first
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      // get the file with the given descriptor
      struct files* ptr = get_file(&thread_current()->files, fd);
      // if not found return -1
      if(ptr == NULL)
      {
        f->eax = -1;
      }
      // return the files's size
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
    // close system call
    case SYS_CLOSE:
    {
      // validate the pointer
      validate_pointer(((int*)f->esp+1));
      int fd = *(((int*)f->esp+1));
      // preform the close operation
      close_file(fd);
    }

  }
}
// a function that validates the given pointer
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
  // checks if the pointer is mapped from virtualto kernel address in the page
  // this function is used instead of lookup_page as it is static
  else if(pagedir_get_page(thread_current()->pagedir, p) == NULL)
  {
    exit(-1);
  }
}


bool 
create(const char* file, unsigned initial_size){

  lock_acquire(&lock);
  bool is_created = filesys_create(file, initial_size);
  lock_release(&lock);
  return is_created;
}

// wrapper for validating pointer and craeting file,
bool 
create_wrapper(void* esp){
  const char* file = get_char_ptr((int*)esp+1);
  validate_pointer(file);
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

// wrapper for validating pointer then call seek.
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

// wrapper to validate pointer and call tell.
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

// wrapper to validate pointer and remove a file.
bool 
remove_wrapper(void* esp){
  char* file = get_char_ptr(((int*) esp +1));
  validate_pointer(file);
  return remove(file);
}


int
 wait(tid_t pid){
  return process_wait(pid);
}
// wrapper to validate pointer and wait for a process.
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

// a function that searches for the descriptor in the given list
// if not found returns null
struct files* get_file(struct list* list, int fd)
{
  struct list_elem* iter = list_begin(list);
  struct files *temp = NULL;
  while(iter != list_end(list))
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
// a function that closes a file with the given descriptor
void close_file(int fd)
{
  // get the files
  struct files *f = get_file(&thread_current()->files, fd);
  // close it and free the resource
  list_remove(&f->elem);
  lock_acquire(&lock);
  file_close(f->file);
  lock_release(&lock);
  free(f); 
}

// a function that preforms the exit system call
void exit(int status)
{
  thread_current()->exit_status = status;
  // close all the open files and free the resuorces
  struct list_elem* iter = list_begin(&thread_current()->files);
  while(iter != list_end(&thread_current()->files))
  {
    struct files* t = list_entry(iter,struct files,  elem);
    lock_acquire(&lock);
    file_close(t->file);
    lock_release(&lock);
    iter = list_next(iter); 
    free(t); 
  }
  // close the executable files
  file_close(thread_current()->open_file);
  printf ("%s: exit(%d)\n",thread_current()->name, thread_current()->exit_status);
  thread_exit();
}
