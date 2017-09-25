#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int parseCommand(char* cmd,char **args);
int executeCommand(char** args, int wait);
int  readFile(char **argv,char **history);
void split();
void printHis(char ** history,int hisSize);
void writeFile();
int isEmpty(char *s) ;
void handle_sigchld();
int getCommand(char *cmd);
int lines =-1;
int wait=1;
int flag=1,hisSize=0;
char* path;
#define MAX_LINE 80
#define MAX_SIZE 10
#define DEFAULT 100000
char* history[DEFAULT];
char* subPath[MAX_LINE/2 + 1];




int main(int argc, char *argv[])
{

    path = getenv("PATH");
    split();
    char cmd[DEFAULT];

    char* fileHistory[MAX_LINE/2 +1];



    int fileSize=0,process;

    for (int index= 0; index<MAX_SIZE ; index++)
    {
        history[index]="";
    }
    if(argc!=2)flag=0;
    if(flag)
    {
        wait=1;
        fileSize=readFile(argv[1],fileHistory);
        if(fileSize==0)
        {
            printf("Invalid directio  or empty file");
            return 0;
        }
        for(int i =0 ; i <fileSize ; i++)
        {
            printf("shell > %s\n",fileHistory[i]);
            process=getCommand(fileHistory[i]);
            if(!process)
            {
                return 0;
            }
        }
        return 0;
    }
    hisSize=readFile("file.txt",history);


    while(1)
    {

        wait=1;
        printf("shell > ");
        if(fgets(cmd, DEFAULT,stdin) == NULL)
        {
            writeFile();
            return 0;
        }

        if(cmd[0] == '\0' ||cmd[0] == '\n')
        {
            continue;
        }


        if(isEmpty(cmd))
        {
            continue;
        }
        process=getCommand(cmd);

        if(!process)
        {
            return 0;
        }

    }

    return 0;

}



int getCommand(char *cmd)
{
    int size;
    if(cmd[0]=='!')
    {

        if(cmd[1]=='!')
        {
            cmd[0]=' ';
            cmd[1]=' ';
            if(isEmpty(cmd) && hisSize!=0)
            {
                printf("shell > %s\n",history[hisSize-1]);
                strcpy(cmd, history[hisSize-1]);

            }
            else
            {
                if (hisSize==0)printf("shell > No such command in history.\n");
                else printf("shell > No such file or directory.\n");
                return 1;
            }

        }
        else
        {
            int num= cmd[1] - '0';
            cmd[0]=' ';
            cmd[1]=' ';
            if(num>9|| num <0  )
            {
                printf("shell > No such command in history.\n");
                return 1;
            }

            else if(isEmpty(cmd) && num<hisSize)
            {

                printf("shell > %s\n",history[num-1]);
                strcpy(cmd,history[num-1]);
            }
            else
            {
                int num2=cmd[2]-'0';
                cmd[2]=' ';
                if(isEmpty(cmd))
                {
                    if(isEmpty(cmd) && num==1 && num2==0 && hisSize==MAX_SIZE)
                    {
                        printf("shell > %s\n",history[9]);
                        strcpy(cmd,history[9]);
                    }
                    else
                    {
                        printf("shell > No such command in history.\n");
                        return 1;
                    }

                }
                else
                {
                    printf("shell > No such file or directory.\n");
                    return 1;
                }
            }
        }
    }
    if(cmd[strlen(cmd)-1] == '\n')
    {
        cmd[strlen(cmd)-1] = '\0';
    }

    if(strcmp(cmd,"history")==0)
    {
        printHis(history,hisSize);
    }
    fflush(stdout);
    if(strcmp(cmd,"exit") == 0 )
    {
        writeFile();
        return 0;
    }

    if(strcmp(cmd,"exit") != 0)
    {
        if(hisSize==MAX_SIZE)
        {
            for(int index=0 ; index<MAX_SIZE-1 ; index++)
            {
                history[index]=strdup(history[index+1]);
            }
            history[hisSize-1]=strdup(cmd);
        }
        else
        {
            history[hisSize]=strdup(cmd);
            hisSize++;
        }
    }
    if(strlen(cmd)>MAX_LINE)
    {
        printf("Maximum command\n");
        return 1;
    }
    if(cmd[strlen(cmd)-1]=='&')
    {
        cmd[strlen(cmd)-1]=' ';
        wait=0;
    }
    if(strcmp(cmd,"history")==0)
    {
        return 1;
    }
    char* args[MAX_LINE/2+1];
    size=parseCommand(cmd,args);
    if(strcmp(args[size-2],"&")==0)
    {
        wait=0;
        size=size-1;
        args[size-2]=NULL;

    }

    if(executeCommand(args,wait) == 0)
    {
        return 0;
    }
}
int parseCommand(char* cmd,char** args)
{
    int size=-1;
    char * token=strtok(cmd," ");
    args[++size]=token;
    while( token != NULL )
    {
        token = strtok(NULL," ");
        args[++size]=token;
    }
    return size+1;
}

int executeCommand(char** args,int wait)
{
    handle_sigchld();
    pid_t pid= fork();
    if(pid==0)
    {


        int index=-1;
        while(subPath[++index]!=NULL)
        {
            char * str ="/";
            char *result = malloc(strlen(subPath[index])+strlen(args[0])+strlen(str)+1);
            strcpy(result, subPath[index]);
            strcat(result, str);
            strcat(result, args[0]);
            execv(result, args);
            free(result);

        }
        // execvp(args[0],args);
        printf("shell>No such file or directry\n");
        return 1;
    }
    else if(pid<0)
    {
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        return 1;
    }
    else
    {
        int childNum;

        if(wait==1)
        {
            waitpid(pid, &childNum, 0);
        }

        return 1;

    }

}

int readFile(char **argv,char **history)
{

    int index=0;

    char *cmds = malloc(sizeof(char) * 1024);

    FILE *file = fopen(argv, "r" );

    if ( file == 0 )
    {
        printf( "Could not open file\n" );
        return 0;
    }
    else
    {
        while (fgets(cmds,DEFAULT,file)!=NULL )
        {
            if(cmds[strlen(cmds)-1] == '\n')
            {
                cmds[strlen(cmds)-1] = '\0';
            }
            if(cmds[0] != '\n' && !isEmpty(cmds))
            {
                history[index++] = strdup(cmds);
            }


        }
    }
    fclose( file );
    free(cmds);
    return index;
}
void writeFile()
{



    FILE *file ;

    if (fopen("file.txt", "w" )==0 )
    {

        printf( "Could not open file\n" );
    }
    else
    {
        file=fopen("file.txt", "w" );
        for(int index=0 ; index<MAX_SIZE ; index++)
        {
            if(history[index]!="")
            {
                fputs(history[index],file);
                fputc('\n',file);
            }
            else
            {
                break;
            }
        }
    }
    fclose( file );

}


void split()
{
    char* token ;
    int index=-1;
    subPath[++index] ="";
    token = strtok( path, ":");
    subPath[++index] = strdup(token);

    while( token != NULL )
    {

        subPath[++index] = strdup(token);
        token = strtok( NULL, ":");
    }

}


void printHis(char ** history,int hisSize)
{
    if(strcmp(history[0],"")==0)
    {
        printf("shell : No histoy \n");
    }
    else
    {
        for(int index=0 ; index< hisSize  ; index++)
        {

            printf("%s\n",history[index]);
        }
    }
}
int isEmpty(char *s)
{
    while (*s != '\0')
    {
        if (!isspace(*s))
            return 0;
        s++;
    }
    return 1;
}
void handle_sigchld()
{
    pid_t pid;
    int status;
    while (1)
    {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0)
            break;
        printf("Reaped child %d\n", pid);
    }
    sleep(1);
}


