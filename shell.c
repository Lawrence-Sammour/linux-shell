#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define DEBUG 1
#define MAXLINELEN 4096
#define MAXARGS 128
#define END_OF_LINE 0
#define SEQ_OP ';'
#define SEQUENCE 1

struct cmd
{
    struct cmd *next;
    int terminator;
    char *exe_path;
    int nargs;
    char *arg[MAXARGS];
};

void *ck_malloc(size_t size)
{

    //allocating specific size of memory depending on passed size parameter.
    void *ret = malloc(size);

    //if it does not point to the first byte in memory allocated block then handle error.
    if (!ret)
    {
        perror("dumbshell:ck_malloc");
        exit(1);
    }

    //return pointer that points to allocated memory.
    return ret;
}

char *skip_to_non_ws(char *p)
{
    int ch;
    while (ch = *p)
    {
        if (ch != ' ' && ch != '\t' && ch != '\n')
            return p;
        ++p;
    }
    return 0;
}

char *skip_to_ws_or_sep(char *p)
{
    int ch;
    while (ch = *p)
    {
        if (ch == ' ' || ch == '\t' || ch == '\n')
            return p;
        if (ch == SEQ_OP)
            return p;
        ++p;
    }
    return 0;
}

struct cmd *parse_commands(char *line)
{
    char *ptr;
    int ix;
    struct cmd *cur;

    ptr = skip_to_non_ws(line);
    if (!ptr)
        return 0;
    cur = ck_malloc(sizeof *cur);
    cur->next = 0;
    cur->exe_path = ptr;
    cur->arg[0] = ptr;
    cur->terminator = END_OF_LINE;
    ix = 1;
    for (;;)
    {
        ptr = skip_to_ws_or_sep(ptr);
        if (!ptr)
        {
            break;
        }
        if (*ptr == SEQ_OP) //ptr == ;
        {
            *ptr = 0;
            cur->next = parse_commands(ptr + 1);
            if (cur->next)
            {
                cur->terminator = SEQUENCE;
            }
            break;
        }
        *ptr = 0;
        ptr = skip_to_non_ws(ptr + 1);
        if (!ptr)
        {
            break;
        }
        if (*ptr == SEQ_OP)
        {
            /* found a sequence operator */
            cur->next = parse_commands(ptr + 1);
            if (cur->next)
            {
                cur->terminator = SEQUENCE;
            }
            break;
        }
        cur->arg[ix] = ptr;
        ++ix;
    }
    cur->arg[ix] = 0;
    cur->nargs = ix;
    return cur;
}

//executing manual commands or switching to system calls.
void execute(struct cmd *clist)
{
    int pid, npid, stat;

    if (strcmp(clist->exe_path, "cd") == 0)
    {

        //if user typed cd without a path return to home directory
        if (clist->arg[1] == NULL)
        {

            if (chdir("/home/larry") == -1)
                perror("error in changing directory to home\n");
        }
        else
        {
            //if user typed in a path change directory to that path.
            if (cdFunction(clist->arg[1]) != 0)
            {

                perror("cd function");
            }
        }
    }

    //Don't get into forking if any of the previous if statements were true.
    else
    {

        pid = fork();

        //error when PID is -1 (parent PID positive integer, child PID 0).
        if (pid == -1)
        {
            perror("dumbshell:fork");
            exit(1);
        }
        if (!pid)
        {

            if (strcmp(clist->exe_path, "pwd") == 0)
            {

                if (pwdFunction() != 0)
                    perror("error in invoking pwd function");

                exit(0);
            }

            if (strcmp(clist->exe_path, "kill") == 0)
            {

                //if the user typed kill with no argument show usage.
                if (clist->arg[1] == NULL)
                {

                    printf("kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]\n");
                    exit(0); //exit child process no need to continue.
                }

                //if the user type kill with one argument this argument could be -l or a pid.
                else if (clist->arg[1] != NULL && clist->arg[2] == NULL)
                {

                    if (strcmp(clist->arg[1], "-l") == 0)
                    {

                        //manual kill command supports three kill signals.
                        printf("1.SIGHUP\n9.SIGKILL \n15.SIGTERM\n");

                        exit(0); //exit child.
                    }

                    else
                    {

                        //if user type kill with process PID send sigterm signal to that PID.
                        if (kill(atoi(clist->arg[1]), SIGTERM) == -1)
                            perror("error in sigterm");

                        exit(0);
                    }
                }

                //in case of user typing two arguments.
                else if (clist->arg[1] != NULL && clist->arg[2] != NULL && clist->arg[3] == NULL)
                {

                    //if two arguments can be parsed then send signal depending on signum to specific PID.
                    if (atoi(clist->arg[1]) != 0 && atoi(clist->arg[2]) != 0)
                    {

                        if (Signals(atoi(clist->arg[1]), atoi(clist->arg[2])) != 0)
                            perror("error invoking Signals func");

                        exit(0);
                    }

                    //in case of being able to parse second argument only.
                    else if (atoi(clist->arg[2]) != 0)
                    {

                        //send specific signal to specific PID depending on signame.
                        if (Signals2(clist->arg[1], atoi(clist->arg[2])) != 0)
                            perror("error invoking Signals2");

                        exit(0);
                    }
                }

                else if (clist->arg[1] != NULL && clist->arg[2] != NULL && clist->arg[3] != NULL)
                {

                    if (atoi(clist->arg[1]) != 0 && atoi(clist->arg[2]) != 0 && atoi(clist->arg[3]) != 0)
                    {

                        if (killSignals(atoi(clist->arg[1]), atoi(clist->arg[2]), atoi(clist->arg[3])) != 0)
                            perror("error invoking killSignals");

                        exit(0);
                    }
                }
                else
                {

                    //in case of not using command proparly print usage instructions
                    printf("kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]\n");
                    exit(0);
                }
            }

            if (clist->exe_path != NULL && clist->arg[1] != NULL && clist->arg[2] != NULL)
            {

                //if user types the pipe symbol, pipe between entered commands.
                if (strcmp("|", clist->arg[1]) == 0)
                {

                    //commands will automatically be switched to system calls and handled by OS.
                    if (piping(clist->exe_path, NULL, clist->arg[2], NULL) != 0)
                        perror("error in invoking piping");
                }

                exit(0);
            }

            if (strcmp("ps", clist->exe_path) == 0 && clist->arg[1] == NULL)
            {

                if (psCommand() != 0)
                    perror("error invoking ps");

                exit(0);
            }

            if (strcmp("ps", clist->exe_path) == 0 && strcmp("-A", clist->arg[1]) == 0)
            {

                if (psACommand() != 0)
                    perror("error invoking ps");

                exit(0);
            }

            //piping between commands with arguments.
            if (clist->exe_path != NULL && clist->arg[1] != NULL && clist->arg[2] != NULL && clist->arg[3] != NULL && clist->arg[4] != NULL)
            {

                if (strcmp("|", clist->arg[2]) == 0)
                {

                    if (piping(clist->exe_path, clist->arg[1], clist->arg[3], clist->arg[4]) != 0)
                    {

                        perror("error piping");
                    }
                }

                exit(0);
            }

            /*if non of the prev if stmts were true replace 
            child process with command execution using OS sys calls*/
            execvp(clist->exe_path, clist->arg); /*pass commands and array of strings*/
            fprintf(stderr, "No such command: %s\n", clist->exe_path);
            exit(1);
        }

        do
        {

            //always wait for child process to not cause zombie process.
            npid = wait(&stat);
            //printf("Process %d exited with status %d\n", npid, stat);
        }

        //always wait for child process when forking (do while loop).
        while (npid != pid);
    }
    switch (clist->terminator)
    {

        //If no kill signal is sent invoke execute method for next command.
    case SEQUENCE:
        execute(clist->next);
    }
}

void free_commands(struct cmd *clist)
{
    struct cmd *nxt;

    do
    {
        nxt = clist->next;

        //dellocate memory to prevent memory leak
        free(clist);
        clist = nxt;

        //while having an allocated memory keep dellocating.
    } while (clist);
}

char *get_command(char *buf, int size, FILE *in)
{

    //if redirected correctly. standard input.
    if (in == stdin)
    {
        char stringArr[3][1000];

        //copy username to index 0.
        strcpy(stringArr[0], "larry@larry-VirtualBox:");

        //always get path and put it in index 1.
        strcpy(stringArr[1], getcwd(stringArr[1], sizeof(stringArr[1])));

        strcpy(stringArr[2], "$$ ");

        char charArr[1100];

        //concat string in index 0 and 1 put them in charArr
        strcpy(charArr, strcat(stringArr[0], stringArr[1]));

        //overwrite char array to contain its contents and contents in index 2 of stringArr.
        strcpy(charArr, strcat(charArr, stringArr[2]));

        //pointer points to content in adress of charrArr.
        char *charArrPtr = &charArr;

        //promt what the pointers points to and redirect to output.
        fputs(charArrPtr, stdout); /* prompt */
    }

    //return promt.
    return fgets(buf, size, in);
}

void main(void)
{
    char linebuf[MAXLINELEN];
    struct cmd *commands;

    //while invoking getting the promt function is not not do the following.
    while (get_command(linebuf, MAXLINELEN, stdin) != NULL)
    {

        //Dismantle and analyse command arguments into segments from linebuf.
        commands = parse_commands(linebuf);
        if (commands)
        {

            //invoke execute to execute commands.
            execute(commands);

            //dellocate reserved command memory block.
            free_commands(commands);
        }
    }
}

int pwdFunction()
{

    char CurrentD[1000];

    //gets cuurent directory
    if (printf("%s\n", getcwd(CurrentD, sizeof(CurrentD))) == -1)
        perror("printing directory");

    return 0; //success
}

int cdFunction(char newDir[1000])
{

    //if changing directory (chdir) returns -1 handle error.
    if (chdir(newDir) == -1)
        perror("error in changing directory");

    return 0; //success
}

/*parameters for signum and PID*/
int Signals(int sigNum, int PID)
{

    if (sigNum == 1)
    {

        //the num of the signal sighup is 1.
        if (kill(PID, SIGHUP) == -1)
            perror("error in sighup");
    }

    if (sigNum == 9)
    {

        //send sigterm signal if num is 9.
        if (kill(PID, SIGTERM) == -1)
            perror("error in sigterm");
    }

    if (sigNum == 15)
    {

        if (kill(PID, SIGKILL) == -1)

            perror("error in sigKill");
    }

    return 0;
}

/*parameters for sigName, PID*/
int Signals2(char sigName[50], int PID)
{

    //compare with ignoring letter case.
    if (strcasecmp(sigName, "sighup") == 0)
    {

        if (kill(PID, SIGHUP) == -1)
            perror("error in sighup");
    }

    if (strcasecmp(sigName, "sigkill") == 0)
    {

        if (kill(PID, SIGKILL) == -1)
            perror("error in sigkill\n");
    }

    if (strcasecmp(sigName, "sigterm") == 0)
    {

        if (kill(PID, SIGTERM) == -1)
            perror("error in sigterm\n");
    }

    return 0;
}

int piping(char comm1[50], char arg1[50], char comm2[50], char arg2[50])
{

    int fileD[2];

    int PID1;

    int PID2;

    pipe(fileD);

    PID1 = fork();

    if (pipe(fileD) == -1)

        perror("Error piping");

    //First child
    if (PID1 == 0)

    {

        //close output file discriptor (no need for reading).
        if (close(fileD[0]) == -1)
            perror("error in closing fileD[1]");

        //duplicate the file discriptor of index 0 to standard input.
        if (dup2(fileD[1], STDOUT_FILENO) == -1)
            perror("error in dup2");

        if (close(fileD[1]) == -1)
            perror("error in closing file discriptor");

        //if one of the piped commands have an argument.
        if (arg1 == NULL)
            execlp(comm1, comm1, NULL);

        //if piped command is a simple command program (e.g pwd).
        else if (arg1 != NULL)
            execlp(comm1, comm1, arg1, NULL);
    }

    else if (PID1 == -1)
    {

        perror("Error in fork");
    }

    else

    {

        PID2 = fork();

        if (PID2 == 0)

        {

            //closing fileD[0] (no need for writing).
            if (close(fileD[1]) == -1)
                perror("closing fileD[1]");

            //redirecting output to fileD[1].
            if (dup2(fileD[0], STDIN_FILENO) == -1)
                perror("dup2");

            //closing fileD after being done with it.
            if (close(fileD[0]) == -1)
                perror("error in closing fileD[1]");

            //If command have no arg.
            if (arg2 == NULL)
                execlp(comm2, comm2, NULL);

            else if (arg2 != NULL)
                execlp(comm2, comm2, arg2, NULL);
        }
        else if (PID2 == -1)
        {

            perror("Fork error");
        }

        if (close(fileD[0]) == -1)
            perror("close fileD[0]");

        if (close(fileD[1]) == -1)
            perror("Close fileD[1]");

        waitpid(-1, NULL, 0);

        //Wait for children
        waitpid(-1, NULL, 0);
    }

    return 0;
}

int killSignals(int PID1, int PID2, int PID3)
{

    kill(PID1, SIGTERM);

    kill(PID2, SIGTERM);

    kill(PID3, SIGTERM);

    return 0;
}

int psCommand()
{

    DIR *dirPtr;
    dirPtr = opendir("/proc");

    int openDisc;

    //contains file discriptors (read only)
    openDisc = open("/dev/fd/0", O_RDONLY);

    char teleTYpwriterString[350];

    //read terminated P form buff
    snprintf(teleTYpwriterString, 400, "%s", ttyname(openDisc));

    //print ps menue
    printf("%7s %5s\t%10s %5s\n", "PID", "TTY", "TIME", "CMD");

    struct dirent *entryPtr;

    //read proc
    while ((entryPtr = readdir(dirPtr)) != NULL)
    {
        int scope;

        scope = 1;

        for (int j = 0; entryPtr->d_name[j]; j++)
        {

            //if no digit returned stop.
            if (!isdigit(entryPtr->d_name[j]))
            {
                scope = 0;
                break;
            }
        }
        if (scope)
        {

            char entryPath[350];

            //read disc path for specific process from buff entryPath
            snprintf(entryPath, 400, "/proc/%s/fd/0", entryPtr->d_name);

            int fileD;

            //permitted to read only from dir
            fileD = open(entryPath, O_RDONLY);

            char *ttyPtr;

            //points to terminal name
            ttyPtr = ttyname(fileD);
            if (ttyPtr)
            {
                if (strcasecmp(ttyPtr, teleTYpwriterString) == 0) //in case process is active (matches term)
                {
                    snprintf(entryPath, 400, "/proc/%s/stat", entryPtr->d_name);

                    FILE *filePtr;

                    //mounted
                    filePtr = fopen(entryPath, "r");

                    int intNum;

                    char comLine[500];

                    char readF;

                    //store cmd PID
                    fscanf(filePtr, "%i%s%c%c%c", &intNum, comLine, &readF, &readF, &readF);

                    //Null (termination)
                    comLine[strlen(comLine) - 1] = '\0';

                    int userModeT;

                    int startT;

                    int finalTime;

                    for (int j = 0; j < 11; j++)

                        //time spent in user mode
                        fscanf(filePtr, "%i", &userModeT);

                    //time spent in kernel mode
                    fscanf(filePtr, "%i", &startT);

                    //find time in secs (convert from jeffies)
                    finalTime = ((userModeT + startT) / sysconf(_SC_CLK_TCK));

                    char time_s[400];

                    //time format (H, minutes, seconds)
                    sprintf(time_s, "%02i:%02i:%02i", (finalTime / 3600) % 3600, (finalTime / 60) % 60, finalTime % 60);

                    //printing ps format
                    printf("%7s %5s\t%10s %5s\n", entryPtr->d_name, ttyPtr + 5, time_s, comLine + 1);

                    //close file
                    fclose(filePtr);
                }
            }

            //close discriptor
            close(fileD);
        }
    }

    //close dir
    close(openDisc);

    //sucess
    return 0;
}

int psACommand()
{

    DIR *dirPtr;
    dirPtr = opendir("/proc");

    int openDisc;

    //contains file discriptors (read only)
    openDisc = open("/dev/fd/0", O_RDONLY);

    char teleTYpwriterString[350];

    //read terminated P form buff
    snprintf(teleTYpwriterString, 400, "%s", ttyname(openDisc));

    //print ps menue
    printf("%7s %5s\t%10s %5s\n", "PID", "TTY", "TIME", "CMD");

    struct dirent *entryPtr;

    //read proc
    while ((entryPtr = readdir(dirPtr)) != NULL)
    {
        int scope;

        scope = 1;

        for (int j = 0; entryPtr->d_name[j]; j++)
        {

            //if no digit returned stop.
            if (!isdigit(entryPtr->d_name[j]))
            {
                scope = 0;
                break;
            }
        }
        if (scope)
        {

            char entryPath[350];

            //read disc path for specific process from buff entryPath
            snprintf(entryPath, 400, "/proc/%s/fd/0", entryPtr->d_name);

            int fileD;

            //permitted to read only from dir
            fileD = open(entryPath, O_RDONLY);

            char *ttyPtr;

            snprintf(entryPath, 400, "/proc/%s/stat", entryPtr->d_name);

            FILE *filePtr;

            //mounted
            filePtr = fopen(entryPath, "r");

            int intNum;

            char comLine[500];

            char readF;

            //store cmd PID
            fscanf(filePtr, "%i%s%c%c%c", &intNum, comLine, &readF, &readF, &readF);

            //Null (termination)
            comLine[strlen(comLine) - 1] = '\0';

            int userModeT;

            int startT;

            int finalTime;

            for (int j = 0; j < 11; j++)

                //time spent in user mode
                fscanf(filePtr, "%i", &userModeT);

            //time spent in kernel mode
            fscanf(filePtr, "%i", &startT);

            //find time in secs (convert from jeffies)
            finalTime = ((userModeT + startT) / sysconf(_SC_CLK_TCK));

            char time_s[400];

            //time format (H, minutes, seconds)
            sprintf(time_s, "%02i:%02i:%02i", (finalTime / 3600) % 3600, (finalTime / 60) % 60, finalTime % 60);

            //points to terminal name
            ttyPtr = ttyname(fileD);

            if (ttyPtr) //if valid tty
            {
                printf("%7s %5s\t%10s %5s\n", entryPtr->d_name, ttyPtr + 5, time_s, comLine + 1);
            }
            else //if non-valid tty
            {
                printf("%7s %5c\t%10s %5s\n", entryPtr->d_name, '?', time_s, comLine + 1);
            }

            //close file
            fclose(filePtr);

            //close discriptor
            close(fileD);
        }
    }

    //close dir
    close(openDisc);

    //sucess
    return 0;
}
