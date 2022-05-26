#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void validate_pointer(void* p);
void exit(int);

#endif /* userprog/syscall.h */
