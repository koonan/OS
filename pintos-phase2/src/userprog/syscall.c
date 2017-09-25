#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <list.h>

static void syscall_handler (struct intr_frame *);
struct lock filesys_lock;
static uint32_t *esp;
void
syscall_init (void) 
{  
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f ) 
{
   esp =  f->esp;
  /* check the validity of the pointers*/
if(!is_valid_pointer(esp) || !is_valid_pointer(esp+1)
  || !is_valid_pointer(esp+2) || !is_valid_pointer(esp+3)){
     exit(-1);
  }
else{
   int  number = *esp ;  //system call number

  switch(number){
    
    case(SYS_HALT): //Takes no arguments
      halt();    
    break;

    case(SYS_EXIT): //one argument
       exit(*(esp+1));
    break;

    case(SYS_EXEC): //one argument
       f->eax = exec((char *) *(esp+1));
    break;

    case(SYS_WAIT): //one argument
       f->eax = wait(*(esp+1));
    break;

    case(SYS_CREATE): //Two arguments
        f->eax = create((char *)*(esp+1),*(esp+2));
    break;

    case(SYS_REMOVE): //one argument
        f->eax = remove((char *)*(esp+1));
    break;

    case (SYS_OPEN):
        f->eax = open ((char *) *(esp + 1));
    break;   

    case(SYS_FILESIZE): //one argument
        f->eax = filesize(*(esp+1));
    break;

    case(SYS_READ): //Three arguments

        f->eax = read(*(esp+1),(void *)*(esp+2),*(esp+3));
    break;

    case(SYS_WRITE): //Three arguments
        f->eax = write(*(esp+1),(void *)*(esp+2),*(esp+3));
    break;

    case(SYS_SEEK): //Two arguments
        seek(*(esp+1),*(esp+2));
    break;

    case(SYS_TELL): //one argument
        f->eax = tell(*(esp+1));
    break;

    case(SYS_CLOSE): //one argument
         close(*(esp+1));
    break;

  }
}
}

/*Terminates Pintos by calling shutdown_power_off() (declared in threads/init.h).
This should be seldom used, because you lose some information about possible 
deadlock situations, etc.*/
void
halt (void) 
{
  shutdown_power_off();
}

/*Terminates the current user program, returning status to the kernel.
If the process's parent waits for it, this is the status that will 
be returned. Conventionally, a status of 0 indicates success and nonzero 
values indicate errors.*/
void
exit (int status)
{
  printf ("%s: exit(%d)\n",thread_current()->name,status); //print exit status
  struct thread *parent = thread_current()->parent;
  struct child *ch;
  if(parent != NULL)
  {
   struct list_elem *e;
   for (e = list_begin (&parent->all_children); e != list_end (&parent->all_children);
       e = list_next (e))
    {
      ch = list_entry (e, struct child, child_elem);
      if(ch->child_id == thread_current()-> tid)
      {
      	  lock_acquire(&parent-> exec_lock);
   		    ch-> is_exited = true; // set the child to be exited
          ch-> exit_status = status; // set the exit status for the child
          lock_release(&parent-> exec_lock);
      }
   }

  }
   thread_exit();
}

/*Runs the executable whose name is given in cmd_line, passing any given arguments,
and returns the new process's program id (pid). Must return pid -1, which otherwise 
should not be a valid pid, if the program cannot load or run for any reason. Thus, 
the parent process cannot return from the exec until it knows whether the child 
process successfully loaded its executable. You must use appropriate synchronization
to ensure this.*/
pid_t
exec (const char *file)
{
  if(!is_valid_pointer(file))
     exit(-1);

  struct thread *current = thread_current();
  current -> load_status = 0;
	tid_t id = process_execute(file);
    
	lock_acquire(&current -> exec_lock);
   
	while(current -> load_status == 0) // wait whenever the load status isnot set
	{
		cond_wait(&current -> wait_cond, &current -> exec_lock); //wait for the child thread 
	}

	if(current -> load_status == -1){ //load fail
         id = -1;
	}

	lock_release(&current -> exec_lock);

   return id;
 
}

/* Waits for a child process pid and retrieves the child's exit status.
If pid is still alive, waits until it terminates. Then, returns the 
status that pid passed to exit. If pid did not call exit(), but was 
terminated by the kernel (e.g. killed due to an exception), wait(pid)
must return -1. It is perfectly legal for a parent process to wait for
child processes that have already terminated by the time the parent calls
wait, but the kernel must still allow the parent to retrieve its child's 
exit status, or learn that the child was terminated by the kernel.

wait must fail and return -1 immediately if any of the following conditions is true:

pid does not refer to a direct child of the calling process. pid is a 
direct child of the calling process if and only if the calling process 
received pid as a return value from a successful call to exec.
Note that children are not inherited: if A spawns child B and B spawns
child process C, then A cannot wait for C, even if B is dead. A call to 
wait(C) by process A must fail. Similarly, orphaned processes are not assigned 
to a new parent if their parent process exits before they do.

The process that calls wait has already called wait on pid. That is, a process
may wait for any given child at most once.Processes may spawn any number of children,
wait for them in any order, and may even exit without having waited for some or all 
of their children */
int
wait (pid_t pid)
{
  return process_wait(pid);
}

/* Creates a new file called file initially initial_size bytes in size. 
Returns true if successful, false otherwise. Creating a new file does not 
open it: opening the new file is a separate operation which would 
require a open system call.*/
bool
create (const char *file, unsigned initial_size)
{ 
  if (!is_valid_pointer (file))
    exit (-1);
  lock_acquire(&filesys_lock);
  bool temp = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return temp;
}

/* Deletes the file called file. Returns true if successful, false otherwise.
A file may be removed regardless of whether it is open or closed,
and removing an open file does not close it.*/
bool
remove (const char *file)
{
  if (!is_valid_pointer (file))
    exit (-1);
  lock_acquire(&filesys_lock);
  bool temp = filesys_remove(file);
  lock_release(&filesys_lock);
  return temp;
}

/* Opens the file called file. Returns a nonnegative integer handle 
called a "file descriptor" (fd), or -1 if the file could not be opened.
File descriptors numbered 0 and 1 are reserved for the console: 
fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output.
The open system call will never return either of these file descriptors, 
which are valid as system call arguments only as explicitly described below.
Each process has an independent set of file descriptors. File descriptors 
are not inherited by child processes.When a single file is opened more than
once, whether by a single process or different processes, each open returns
a new file descriptor. Different file descriptors for a single file are closed
independently in separate calls to close and they do not share a file position.*/
int
open (const char *file)
{
   
  if (!is_valid_pointer (file))
    exit (-1);
  struct file_descriptor *fd;
  int return_value;
  lock_acquire(&filesys_lock);
  struct file *temp = filesys_open(file);
  /* allocate the file descriptor struct and set its atrributes */
  if(temp != NULL){
  fd = calloc (1, sizeof *fd);
  fd -> file_name = file;
  fd -> opened_file = temp ;
  fd -> id = thread_current()-> fd;;
  return_value = fd -> id;
  thread_current()->fd ++;
  list_push_back(&thread_current()->opened_files, &fd->file_elem); // push file into opened files list
  }else{
  	return_value = -1 ;
  }
  lock_release(&filesys_lock);
  
  return return_value;
}

/*Returns the size, in bytes, of the file open as fd.*/
int
filesize (int fd) 
{
   int size = -1;
   lock_acquire(&filesys_lock);
   struct file *f = get_file(fd)->opened_file;
   if(f != NULL)
      size = file_length(f);
   lock_release(&filesys_lock); 
   return size;
}

/* Reads size bytes from the file open as fd into buffer. Returns 
the number of bytes actually read (0 at end of file), or -1 if
the file could not be read (due to a condition other than end 
of file). Fd 0 reads from the keyboard using input_getc().*/
int
read (int fd, void *buffer, unsigned size)
{
  if(!is_valid_pointer(buffer)|| !is_valid_pointer(buffer+size)){
    exit(-1);
  }
 //--------------------------------------------------------//
  lock_acquire(&filesys_lock); 
  if (fd == STDOUT_FILENO){ // related to write system call
    lock_release(&filesys_lock);
    return -1 ;
  }
 //-------------------------------------------------------//
	int bytes_size = -1;
  if(fd == STDIN_FILENO){ // read from keyboard
    unsigned i ;
    uint8_t *buf = (uint8_t *) buffer;
    for (i = 0; i < size; i++)
    {
      buf[i] = input_getc();
    }
    lock_release(&filesys_lock);
    return size ;
  }
 //------------------------------------------------------//
  struct file *f = get_file(fd)->opened_file; // read from file given its fd
  if(f != NULL)
    bytes_size = file_read(f, buffer, size);	
  lock_release(&filesys_lock);

  return bytes_size;
}

/* Writes size bytes from buffer to the open file fd. 
Returns the number of bytes actually written, which may
be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the file, 
but file growth is not implemented by the basic file system. 
The expected behavior is to write as many bytes as possible up 
to end-of-file and return the actual number written, or 0 if no 
bytes could be written at all.Fd 1 writes to the console.*/
int
write (int fd, const void *buffer, unsigned size)
{ 
  if(!is_valid_pointer(buffer) || !is_valid_pointer(buffer+size)){
    exit(-1);
  }	
  //-------------------------------------------------------------//
   lock_acquire(&filesys_lock);
  if (fd == STDIN_FILENO){ // related to read system call
      lock_release(&filesys_lock);
      return -1 ;   
      }  
  //------------------------------------------------------------//      
  if (fd == STDOUT_FILENO){ // writes to the console.
    putbuf(buffer , size);
    lock_release(&filesys_lock);
    return size ;
  }
  //-------------------------------------------------------------//
  int bytes_size = -1;
  struct file *f = get_file(fd)->opened_file;
  if(f != NULL)
    bytes_size = file_write(f, buffer, size);	
  lock_release(&filesys_lock);
  return bytes_size;
}

/* Changes the next byte to be read or written in open file 
fd to position, expressed in bytes from the beginning of the
file. (Thus, a position of 0 is the file's start.)*/
void
seek (int fd, unsigned position) 
{
  lock_acquire(&filesys_lock);
  struct file *f = get_file(fd)->opened_file;
  if(f != NULL)
      file_seek(f, position);	
  lock_release(&filesys_lock);
}

/* Returns the position of the next byte to be read or written
in open file fd, expressed in bytes from the beginning of the file.*/
unsigned
tell (int fd) 
{
  unsigned position = 0;
  lock_acquire(&filesys_lock);
  struct file *f = get_file(fd)->opened_file;
  if(f != NULL)
    position = file_tell(f);	
  lock_release(&filesys_lock);

  return position;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly
closes all its open file descriptors, as if by calling this function for each one.*/
void
close (int fd)
{
  lock_acquire(&filesys_lock);
  close_open_file(fd);
  lock_release(&filesys_lock);
}

/* check if the pointer is valid :
The validity of user pointer means that it satisfies 3 conditions
1) It is not NULL.
2) It is mapped.
3) It is within physical_base. */
bool
is_valid_pointer (const void *usr_ptr)
{
  if(usr_ptr != NULL && is_user_vaddr (usr_ptr)){

   		if(pagedir_get_page(thread_current()->pagedir, usr_ptr) != NULL){

   			return true;

  		 }else{

  		 	return false;

  		 }
   }
	
return false;
}

/* get the file specified by a given fd from opened files list */
struct file_descriptor *get_file(int fd){
   struct  file_descriptor *temp;
   struct list_elem *e;
	 for (e = list_begin (&thread_current()->opened_files); e != list_end (&thread_current()->opened_files);
       e = list_next (e)){

         temp = list_entry (e, struct file_descriptor, file_elem);
         if(temp -> id == fd)
         	return temp ;
	 }

	 return NULL;
}

/* close specified file by its given fd , remove it fron opened files 
list and free its struct*/
void close_open_file (int fd){

   struct  file_descriptor *temp;
   struct list_elem *e;
   for (e = list_begin (&thread_current()->opened_files); e != list_end (&thread_current()->opened_files);
       e = list_next (e)){

         temp = list_entry (e, struct file_descriptor, file_elem);
         if (temp->id == fd){
           list_remove(&temp->file_elem);
           file_close(temp->opened_file);
           free(temp);
           break;
        }
}
}

