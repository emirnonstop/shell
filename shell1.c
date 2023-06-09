/* <Команда Shellа >
→ < Команда с условным выполнением >{ [; | &] < Команда Shellа>}{ ; |&} <Команда с условным выполнением > 
→ <Команда> { [&& | || ] <Команда с условным выполнением>} <Команда> 
→ {<перенаправление ввода/вывода>}<Конвейер> | <Конвейер>{<перенаправление ввода/вывода>} | ( <Команда Shellа>) <перенаправление ввода/вывода> 
→ {<перенаправление ввода > } <перенаправление вывода> | {<перенаправление вывода>}<перенаправление ввода > <перенаправление ввода > 
→ ‘<’ файл <перенаправление вывода> → ‘>’ файл | ‘>>’ файл <Конвейер>→ <Простая команда> {‘|’ <Конвейер>} <Простая команда>→ <имя команды><список аргументов>*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#define MAX_COMMANDS 100

int shellCommand(int startPointer, int endPointer, char *cmds[]);

char *getCommandLine(int *length) {
    *length = 0;
    int capacity = 1;
    char *str = (char *)malloc(sizeof(char));
    if (str == NULL){ 
        perror("malloc");
        exit(1);
    }
    char c;
    scanf("%c", &c);
    while ((c != EOF) && (c != '\n')){ 
        str[(*length)++] = c;
        if (*length >= capacity) {
            capacity *= 2; 
            str = (char*)realloc(str, capacity * sizeof(char));
            if (!str){ 
                perror("realloc");
                exit(1);
            }
        }
        scanf("%c", &c);      
    }
    str[*length] = '\0';
    return str;
}

char *removeSpaces(char *cmdLine, int length){
    char *str = (char *)malloc(sizeof(char) * length);
    int i = 0;
    if (*cmdLine == ' '){ 
        for (i = 0; i < length; i++){
            if (*(cmdLine + i) != ' '){ 
                break;
            }
        }
    }
    int flag = 0;
    int k = 0; 
    for (int j = i; j < length; j++){
        if (*(cmdLine + j) != ' '){
            if (flag){ 
                *(str + k) = ' ';
                k++;
            }  
            *(str + k) = *(cmdLine + j);
            k++;
            flag = 0; 
        } else { 
            flag++;
        }
    }
    free(cmdLine);
    return str;
}

char **scanMy(char *cmdLine, char *cmds[], int length, int *argcArray){
    int internalIndex = 0;
    *argcArray = 0;
    int capacity = 1;
    int flag, flag1;
    flag = flag1 = 0;
    for (int i = 0; i < length; i++){
        
        if ((cmdLine[i] != '|') && (cmdLine[i] != ';') && (cmdLine[i] != '&') && (cmdLine[i] != '>') && (cmdLine[i] != '<') && (cmdLine[i] != ' ') && (cmdLine[i] != '(') && (cmdLine[i] != ')')){
            if (flag1){
                cmds[(*argcArray)][internalIndex++] = '\0';
                internalIndex = 0;
                flag1 = 0;
                capacity = 1;
                (*argcArray)++;
            }
            if (!flag){
                cmds[(*argcArray)] = (char *)malloc(sizeof(char));
                flag = 1;
            }
            cmds[(*argcArray)][internalIndex++] = cmdLine[i];
            if (internalIndex >= capacity){
                capacity *= 2;
                cmds[(*argcArray)] = (char*)realloc(cmds[(*argcArray)], capacity * sizeof(char));
            }
        } else {
            if (flag){
                cmds[(*argcArray)][internalIndex++] = '\0';
                internalIndex = 0;
                flag = 0;
                capacity = 1;
                (*argcArray)++;
            }
            if (cmdLine[i] != ' '){
                if(!flag1){
                    cmds[(*argcArray)] = (char *)malloc(sizeof(char));
                    flag1 = 1;
                }
                cmds[(*argcArray)][internalIndex++] = cmdLine[i];
                if (internalIndex >= capacity){
                    capacity *= 2;
                    cmds[(*argcArray)] = (char*)realloc(cmds[(*argcArray)], capacity * sizeof(char));
                }
            }else{
                if (flag1){
                        cmds[(*argcArray)][internalIndex++] = '\0';
                        internalIndex = 0;
                        flag1 = 0;
                        capacity = 1;
                        (*argcArray)++;
                }
            }
        }
    }
    if (cmds[*(argcArray)] == NULL){
        (*argcArray)--;
    }
    return cmds;
}

int redirection(int cmdPointer, char *cmds[]) {
    while(cmds[cmdPointer] == NULL){ ///🙈🙈🙈🙈🙈🙈
        cmdPointer++;
    }
    if (strcmp(cmds[cmdPointer], ">") == 0) {
        cmdPointer++;
        int fdec = open(cmds[cmdPointer], O_WRONLY | O_CREAT | O_TRUNC);
        dup2(fdec, 1);
        return 0;
    } else if (strcmp(cmds[cmdPointer], "<") == 0) {
        cmdPointer++;
        int fdec = open(cmds[cmdPointer], O_RDONLY);
        dup2(fdec, 0);
        return 0;
    } else if (strcmp(cmds[cmdPointer], ">>") == 0) {
        cmdPointer++;
        int fdec = open(cmds[cmdPointer], O_WRONLY | O_CREAT | O_APPEND);
        dup2(fdec, 1);
        return 0;
    } else {
        return 1;
    }
}

int transporter(int start, int end, char *cmds[]){
    pid_t currentPID, pid;
    int fdec, status, currentStatus, index, flag, quantityOfCommands;
    int argc = 0;
    int newEnd = start;
    int endPointer = end;
    int newStart = start;
    flag = quantityOfCommands = 0; 
    while(cmds[start] == NULL){ 
        start++;
    }
    if (cmds[start] != NULL){ 
        for (int i = start; i < end; i++){ 
            if (cmds[i] == NULL){ 
                endPointer = i;
            }
        }
    }
    int i;
    newEnd = start;
    newStart = start;
    while (!flag){
        int flag2 = 0;
        for (i = newStart; i < endPointer; i++){
            if ((strcmp(cmds[i], "|") != 0) && (strcmp(cmds[i], ">") != 0 ) && (strcmp(cmds[i], "<") != 0)){ 
                newEnd++;
            } else if((strcmp(cmds[i], "|") == 0) || (strcmp(cmds[i], ">") == 0 ) || (strcmp(cmds[i], "<") == 0) || (strcmp(cmds[i],">>") == 0)){
                flag2 = 1;
                break;
            }
        }
        if (strcmp(cmds[newEnd - 1], "&") == 0) newEnd--;
        quantityOfCommands++;
        //создаем аргументы для exec
        //if (newStart == newEnd) newEnd ++;
        if (strcmp(cmds[newEnd - newStart], ")") == 0){ 
            newEnd--;
        }
       // printf("%s\n", cmds[newEnd]);
        char *execArray[newEnd - newStart + 1];
        for (int i = 0; i < (newEnd - newStart); i++){ 
            execArray[i] = cmds[newStart + i]; 
            //printf("%s\n", execArray[i]);
        }
        execArray[newEnd - newStart] = NULL;
        if (i == end){ 
            flag = 1;
        } else{ 
            flag = 0;
            newStart = newEnd + 1; //
            newEnd = newStart;
        }
        //printf("flag = %d\n", flag);
        int fd[2];
        if (!flag) pipe(fd);
        if ((pid = fork()) < 0){ 
            perror("fork traspoter");
            exit(1);
        }
        if (!pid){
            // son-processes
            if (quantityOfCommands > 1){ // если до этого были команды => перенаправление чтения 
                dup2(fdec, 0);
            }
            if(!flag){  
                close(fd[0]);
                dup2(fd[1], 1);             // перенапраление записи для след команды конвейера 
            }
            execvp(execArray[0], execArray);
        } else { 
            //dad-processes
            if (quantityOfCommands > 1){ 
                close(fdec);
            }
            if (!flag){
                close(fd[1]);
            }
            fdec = fd[0]; 
        }
    }
    while((currentPID = wait(&status)) != -1){
        if(currentPID == pid)
            currentStatus = WEXITSTATUS(status);
    }    
    if(currentStatus) currentStatus = 1;
    return currentStatus;
}

void hndlr(int s){ 
    s = s + 1 ;
}
int command(int startPointer, int endPointer, char *cmds[]){ 
    pid_t pid;
    int status, flag1; 
    int flag = flag1 = 0;
    int cmdPointer = startPointer;
    int checkPointer = startPointer;
    int endOfTransporter = endPointer;
    int openBrace = 0; 
    signal(SIGUSR1, hndlr);
    for (int i = startPointer; i < endPointer; i++){ 
        if ((strcmp(cmds[i], "(")) == 0 ){
            if (cmdPointer == checkPointer) cmdPointer = i + 1;
            openBrace++;
        } else if (strcmp(cmds[i], ")") == 0 ){
            openBrace--;
            if (openBrace == 0){
                checkPointer = cmdPointer;
                if ((pid = fork()) == 0) {
                    status = shellCommand(cmdPointer, i, cmds);
                    exit(status);
                }
                waitpid(pid, &status, 1);
                flag = 1;
                if (i == (endPointer - 1)) flag1 = 1;
                kill(getpid(), SIGUSR1);
                //  могу обнулить здесть  "( ююю )""
                for (int j = cmdPointer - 1 ; j <= i; j++){ 
                    free(cmds[j]);
                    cmds[j] = NULL;
                }
            }
        }
    }
    //check /////////
    while((cmds[startPointer] == NULL) && (startPointer != endPointer)){ 
        startPointer++;
    }
    if (flag1) return 0;
    //printf("flag == %d\n", flag);
    if (flag) pause();
    if ((pid = fork()) < 0){ 
        perror("fork");
        exit(1);
    } 
    cmdPointer = startPointer;
    if (!pid){ 
        if(!(redirection(cmdPointer, cmds))){ 
            cmdPointer += 2;
            if(!(redirection(cmdPointer, cmds))){ 
                cmdPointer += 2;
            }
            status = transporter(cmdPointer, endPointer, cmds);
            if (status){ 
                exit(1);
            } else { 
                exit(0);
            }
        } else { 
            //printf("startPointer in the transporter == %d, endOfTransporter == %d\n", startPointer, endOfTransporter);
            for (int i = startPointer; i < endPointer; i++){ 
                if (cmds[i] != NULL){ 
                    if ((strcmp(cmds[i], ">>") == 0) || (strcmp(cmds[i], ">") == 0) || (strcmp(cmds[i], "<") == 0)) {
                        endOfTransporter = i; 
                        break;
                    }
                }
            }
            if (endOfTransporter != endPointer){
                redirection((endOfTransporter), cmds);
                redirection((endOfTransporter + 2), cmds); 
            }
            //printf("startPointer in the transporter == %d, endOfTransporter == %d\n", startPointer, endOfTransporter);
            status = transporter(startPointer, endOfTransporter, cmds);
            //printf("status Transporter = %d\n", status);
            if (status){ 
                exit(1);
            } else { 
                exit(0);
            }
        }
    }
    if (strcmp(cmds[endOfTransporter - 1], "&") != 0) wait(&status);
    for(int i = startPointer; i < endPointer; i++){ 
        if (cmds[i] != NULL){ 
            free(cmds[i]);
            cmds[i] = NULL;
        }
    }
    int currentStatus = WEXITSTATUS(status);
    //printf("success-return in the command = %d\n", currentStatus);
    //if (currentStatus) currentStatus = 0;
    //else currentStatus = 1;
    return(currentStatus);
}

int conditionCommand(int startPointer, int endPointer, char *cmds[]){ 
    int waitSuccessOfFirst = -1;
    int endOfCommand = endPointer;
    for (int i = startPointer; i < endPointer; i++){
        if (strcmp(cmds[i], "&&") == 0){ 
            waitSuccessOfFirst = 1;
            endOfCommand = i;
            break;
        } else if (strcmp(cmds[i], "||") == 0){ 
            waitSuccessOfFirst = 0;
            endOfCommand = i;
            break;
        }
    }
    int status = command(startPointer, endOfCommand, cmds);
    if (((waitSuccessOfFirst == 1) && (status == 0)) || ((waitSuccessOfFirst == 0) && (status == 1))){ 
        endOfCommand ++;
        status = conditionCommand(endOfCommand, endPointer, cmds);
        //wait(NULL);
    }
    if (status) status = 1;
    return(status);
}

int shellCommand(int startPointer, int endPointer, char *cmds[]){ 
    int cmdPointer = startPointer;
    pid_t pid, pid1;
    int status, i;
    int flagFon = 0, flag = 0, flagPos = 0;
    for (i = startPointer; i < endPointer; i++){
        //printf(" 1 i = %d %s\n",i, cmds[i]);
        if (strcmp(cmds[i], "&") == 0) flagFon = 1;
        else if(strcmp(cmds[i], ";") == 0) flagPos = 1;
    }
    for (i = startPointer; i < endPointer; i++){ 
        if (strcmp(cmds[i], "&") == 0){ 
            //запускаем фоновый режим 
            if ((pid = fork()) < 0){ 
                perror ("fork");
                exit(1);
            }
            if(!pid){ 
                if ((pid1 = fork()) < 0){ 
                    perror ("fork");
                    exit(1);
                }
                if (!pid1){
                    //внук 
                    printf("i perform fon mode\n");
                    signal(SIGINT, SIG_IGN);
                    int fdec0, fdec1;
                    fdec0 = open("/dev/null", O_RDONLY);
                    fdec1 = open("/dev/null", O_WRONLY);
                    dup2(fdec0, 0);
                    dup2(fdec1, 1);
                    int status = conditionCommand(cmdPointer, i , cmds);
                    exit(status);                
                } else {
                    exit(0);
                }
            }else{ 
                wait(NULL);
                if (i != (endPointer - 1)){
                    cmdPointer = i + 1;
                }
                flag = 1;
                break;
            }
        } else if (strcmp(cmds[i], "&&") == 0){
            if (!flagFon) break;
        } else if (strcmp(cmds[i], "||") == 0){
            if (!flagFon) break;
        } else if (strcmp(cmds[i], ";") == 0){
            status = conditionCommand(startPointer, i, cmds);
            if (i != (endPointer - 1)){ 
                cmdPointer = i + 1;
            }
            free(cmds[i]);
            cmds[i] = NULL;
            break;
        } 
    }
    if ((cmdPointer != startPointer) && ( cmdPointer != endPointer - 1)){
        status = shellCommand(cmdPointer, endPointer, cmds);
    } else {
        if (!flag) status = conditionCommand(startPointer, endPointer, cmds);
    }
    return status;
}



int main(int argc, char *argv[]) { 
    int length = 0; 
    int argcArray;
    while (1){ 
        //printf("emirkhatmullin >");
        fflush(0);
        char *cmdLine = getCommandLine(&length); // получили командную строку
        if (!length){
            printf("command line is empty\n");
            continue;
        }
        cmdLine = removeSpaces(cmdLine, length); //удаляем лишние пробелы 
        char *cmds[MAX_COMMANDS];                //массив из строк-команд/символов
        length = strlen(cmdLine);

        scanMy(cmdLine, cmds, length, &argcArray);
        /*// ================cheack: PARSING A CMDLINE================
        for (int i = 0; i <= argcArray; i++){
            printf("%s\n", cmds[i]);
        }
        //} ==========================================================*/
        argcArray++;
        shellCommand(0, argcArray, cmds);
        free(cmdLine);
        //wait(NULL);
        fflush(0);
        
    }
    return 0;
}
