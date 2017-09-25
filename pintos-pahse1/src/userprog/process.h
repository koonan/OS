#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
static void push_cmd(const char *file_name , void **esp , char** save_ptr);
void process_close_files ();
void process_remove_children();
struct child
{
   tid_t child_id;               /*the id of the child*/
   struct list_elem child_elem;  /*list representation of the child*/
   bool is_waited_on;            /*To check whether the parent has waited on the child before*/
   bool is_exited;               /*To check if the child finished waiting*/
   int exit_status ;             /*0 if success , fail o.w*/
};
#endif /* userprog/process.h */
