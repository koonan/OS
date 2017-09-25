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

void
halt (void) 
{
  shutdown_power_off();
}

void
exit (int status)
{
  printf ("%s: exit(%d)\n",thread_current()->name,status);
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
   		    ch-> is_exited = true;
          ch-> exit_status = status;
          lock_release(&parent-> exec_lock);
      }
   }

  }
   thread_exit();
}

pid_t
exec (const char *file)
{
  if(!is_valid_pointer(file))
     exit(-1);

  struct thread *current = thread_current();
  current -> load_status = 0;
	tid_t id = process_execute(file);
    
	lock_acquire(&current -> exec_lock);
   
	while(current -> load_status == 0)
	{
		cond_wait(&current -> wait_cond, &current -> exec_lock); //wait for the child thread 
	}

	if(current -> load_status == -1){ //load fail
         id = -1;
	}

	lock_release(&current -> exec_lock);

   return id;
 
}
int
wait (pid_t pid)
{
  return process_wait(pid);
}

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

int
open (const char *file)
{
   
  if (!is_valid_pointer (file))
    exit (-1);
  struct file_descriptor *fd;
  int return_value;
  lock_acquire(&filesys_lock);
  struct file *temp = filesys_open(file);
  if(temp != NULL){
  fd = calloc (1, sizeof *fd);
  fd -> file_name = file;
  fd -> opened_file = temp ;
  fd -> id = thread_current()-> fd;;
  return_value = fd -> id;
  thread_current()->fd ++;
  list_push_back(&thread_current()->opened_files, &fd->file_elem);
  }else{
  	return_value = -1 ;
  }
  lock_release(&filesys_lock);
  
  return return_value;
}

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

int
read (int fd, void *buffer, unsigned size)
{
  if(!is_valid_pointer(buffer)|| !is_valid_pointer(buffer+size)){
    exit(-1);
  }

  check_valid_buffer(buffer,size);

  lock_acquire(&filesys_lock); 
  if (fd == STDOUT_FILENO){
    lock_release(&filesys_lock);
    return -1 ;
  }
	int bytes_size = -1;
  if(fd == STDIN_FILENO){
    unsigned i ;
    uint8_t *buf = (uint8_t *) buffer;
    for (i = 0; i < size; i++)
    {
      buf[i] = input_getc();
    }
    lock_release(&filesys_lock);
    return size ;
  }
 
  struct file *f = get_file(fd)->opened_file;
 
  if(f != NULL)
    bytes_size = file_read(f, buffer, size);	
  lock_release(&filesys_lock);

  return bytes_size;
}

int
write (int fd, const void *buffer, unsigned size)
{ 
  if(!is_valid_pointer(buffer) || !is_valid_pointer(buffer+size)){
    exit(-1);
  }	
   check_valid_buffer(buffer,size);
   lock_acquire(&filesys_lock);
  if (fd == STDIN_FILENO){
      lock_release(&filesys_lock);
      return -1 ;   
      }     
  if (fd == STDOUT_FILENO){
    putbuf(buffer , size);
    lock_release(&filesys_lock);
    return size ;
  }
  int bytes_size = -1;
  
  struct file *f = get_file(fd)->opened_file;
  
  if(f != NULL)
    bytes_size = file_write(f, buffer, size);	
  lock_release(&filesys_lock);
  return bytes_size;
}

void
seek (int fd, unsigned position) 
{
   lock_acquire(&filesys_lock);
  struct file *f = get_file(fd)->opened_file;
 
  if(f != NULL)
      file_seek(f, position);	
  lock_release(&filesys_lock);
}

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

void
close (int fd)
{
  lock_acquire(&filesys_lock);
  close_open_file(fd);
  lock_release(&filesys_lock);
}


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

/*int user_to_kernel(const void *pointer){
  if(!is_valid_pointer(pointer))
    exit(-1);
  void *ptr  = pagedir_get_page(thread_current()->pagedir,pointer);
  if(!ptr)
    exit(-1);
  return (int) ptr ;
}*/

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


void check_valid_buffer (void* buffer, unsigned size)
{
  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      if(!is_valid_pointer((const void*) local_buffer))
        exit(-1);
      local_buffer++;
    }
}
