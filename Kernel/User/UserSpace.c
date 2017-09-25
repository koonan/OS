#include <stdio.h>
#include <string.h>
#include <stdlib.h>



// Check existance of file
int file_exists(const char *filename)
{
    FILE *fp = fopen (filename, "r");
    if (fp!=NULL) fclose (fp);
    return (fp!=NULL);
}

// Read 0 or 1 from file
void readFile(int index)
{
   
    FILE *file;
    switch(index)
    {
    case 0:
        if(file_exists("/sys/kernel/kobject_safaa/scroll"))
        {
            file=fopen("/sys/kernel/kobject_safaa/scroll","r");
        }
        else
        {
            fclose(file);
            return ;
        }
        break;
    case 1:
        if(file_exists("/sys/kernel/kobject_safaa/num"))
        {
            file=fopen("/sys/kernel/kobject_safaa/num","r");
        }
        else
        {
            fclose(file);
            return ;
        }
        break;
    case 2:
        if(file_exists("/sys/kernel/kobject_safaa/caps"))
        {
            file=fopen("/sys/kernel/kobject_safaa/caps","r");
        }
        else
        {
            fclose(file);
            return ;
        }
        break;
    }
    char buff[100];
    fscanf(file,"%s",buff);
    if(strcmp(buff,"0")==0){
     printf("OFF\n");    
    }
    else{
      printf("ON\n");
    }
    fclose(file);
    return ;
}

// Write 0 or 1 to file
int writeFile(int index,int flag)
{
    FILE *file;
    int state=0;

    switch(index)
    {
    case 0:
        if(file_exists("/sys/kernel/kobject_safaa/scroll"))
        {
            file=fopen("/sys/kernel/kobject_safaa/scroll","w+");
        }
        else
        {
            fclose(file);
            return state;
        }
        break;
    case 1:
        if(file_exists("/sys/kernel/kobject_safaa/num"))
        {
            file=fopen("/sys/kernel/kobject_safaa/num","w+");
        }
        else
        {
            fclose(file);
            return state;
        }
        break;
    case 2:
        if(file_exists("/sys/kernel/kobject_safaa/caps"))
        {
            file=fopen("/sys/kernel/kobject_safaa/caps","w+");
        }
        else
        {
            fclose(file);
            return state;
        }
        break;
    }

    if(flag==1)
    {
        char * buff = "1";
        fputs(buff, file);
    }
    else
    {
        char * buff = "0";
        fputs(buff, file);
    }

    fclose(file);
    return 1;
}

// Them main function to get the commad from the user and set or get the state of the buttons
int main(int argc, char* subStr[])
{
    int index=-1;
    int state;

    if(subStr[1]==NULL || subStr[2]==NULL)
    {
        printf("ERROR COMMAND\n");
        return 0;
    }
    if(strcmp(subStr[2],"num")==0)
    {
        index=1;
    }
    else if (strcmp(subStr[2],"caps")==0)
    {
        index=2;
    }
    else if(strcmp(subStr[2],"scroll")==0)
    {
        index=0;
    }
    else
    {
        printf("Error\n");
        return 0;
    }
    if(subStr[3]!=NULL)
    {


        if(strcmp(subStr[1],"set")==0)
        {
            int ret;

            if (strcmp(subStr[3],"on")==0)
            {
                ret =writeFile(index,1);
            }
            else if (strcmp(subStr[3],"off")==0 )
            {
                ret=writeFile(index,0);

            }
            else
            {
                printf("Connot open the file/n");
                return 0;
            }
            if(ret==0)
                printf("Connot open the file/n");
            return 0;
        }
    }
    else if(subStr[3]==NULL)
    {
        if (strcmp(subStr[1],"get")==0 )
        {

           readFile(index);
            return 0;
          
        }

    }
    else
    {
        printf("Error\n");
        return 0;

    }


    return 0;
}



