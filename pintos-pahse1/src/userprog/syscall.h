#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include <debug.h>
#include "threads/interrupt.h"
#include "filesys/filesys.h"
typedef int pid_t;
void syscall_init (void);
static void syscall_handler (struct intr_frame *f) ;
struct file_descriptor *get_file(int fd);
void close_open_file (int fd);
void check_valid_buffer (void* buffer, unsigned size);
bool is_valid_pointer (const void *usr_ptr);
void halt (void) ;
void exit (int status);
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
