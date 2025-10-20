#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define buffersize sizeof(char)*1024
#define arraySize 10
#define buffersizeb sizeof(char*)

typedef struct SimpleCommand{
   int numberOfAvailableArguments;
   int numberOfArguments;
   int maxArguments;
   char** _arguments;
}SimpleCommand;

SimpleCommand newSimpleCommand(){
   SimpleCommand s;
   s.numberOfArguments=0;
   s.maxArguments=arraySize;
   s._arguments=(char**)malloc(buffersizeb*s.maxArguments);
   return s;   
}


void insertArgument(char* argument,SimpleCommand* simpleCommand){
   char* copy=strdup(argument);
   if(simpleCommand->numberOfArguments<simpleCommand->maxArguments){
      simpleCommand->_arguments[simpleCommand->numberOfArguments++]=copy;
   }else{
      simpleCommand->maxArguments+=arraySize;
      simpleCommand->_arguments=realloc(simpleCommand->_arguments,buffersizeb*simpleCommand->maxArguments);
      simpleCommand->_arguments[simpleCommand->numberOfArguments++]=copy;

   }
}
void printArguments(SimpleCommand s){
   for(int i=0;i<s.numberOfArguments;i++){
      printf("%s\n",s._arguments[i]);
   }
}


typedef struct Command{
   int _numberOfAvailableSimpleArguments;
   int _numberOfSimpleCommands;
   int maxArguments;
   SimpleCommand* _simpleCommands;
   char* _outFile;
   char* _inputFile;
   char* _errFile;
   int _background;

}Command;

Command newCommand(){
   Command c;
   c._numberOfSimpleCommands=0;
   c._simpleCommands=malloc(sizeof(SimpleCommand)*arraySize);
   c.maxArguments=arraySize;
   c._background=0;
   c._errFile=NULL;
   c._inputFile=NULL;
   c._outFile=NULL;
   return c;
}

void insertSimpleCommand(Command* command,SimpleCommand simpleCommand){
   if(command->_numberOfSimpleCommands < command ->maxArguments){
      command->_simpleCommands[command -> _numberOfSimpleCommands++]=simpleCommand;
   }else{
      command->maxArguments+=10;
      command->_simpleCommands=realloc(command->_simpleCommands,command->maxArguments*sizeof(SimpleCommand));
      command->_simpleCommands[command -> _numberOfSimpleCommands++]=simpleCommand;
   }
}


void printCommand(Command c) {
   printf("Command with %d simple commands:\n", c._numberOfSimpleCommands);
   for (int i = 0; i < c._numberOfSimpleCommands; i++) {
      printArguments(c._simpleCommands[i]);
   }
   if (c._outFile) printf("Output file: %s\n", c._outFile);
   if (c._inputFile) printf("Input file: %s\n", c._inputFile);
   if (c._errFile) printf("Error file: %s\n", c._errFile);
   if (c._background) printf("Runs in background\n");
}

Command _currentCommand;
SimpleCommand* _currentSimpleCommand;

Command parseCommand(char *line) {
   Command cmd = newCommand();
   SimpleCommand current = newSimpleCommand();

   char *token = strtok(line, " \t\n");
   while (token != NULL) {
      if (strcmp(token, "|") == 0) {
         // End of one command, start next
         insertSimpleCommand(&cmd,current);
         current = newSimpleCommand();
      } else if (strcmp(token, "<") == 0) {
         token = strtok(NULL, " \t\n");
         cmd._inputFile = strdup(token);
      } else if (strcmp(token, ">") == 0) {
         token = strtok(NULL, " \t\n");
         cmd._outFile = strdup(token);
      } else if (strcmp(token, "2>") == 0) {
         token = strtok(NULL, " \t\n");
         cmd._errFile = strdup(token);
      } else if (strcmp(token, "&") == 0) {
         cmd._background = 1;
      } else {
         insertArgument(token, &current);
      }
      token = strtok(NULL, " \t\n");
   }

   if (current.numberOfArguments > 0)
      insertSimpleCommand(&cmd,current);

   return cmd;
}


void executeCommand(Command *cmd) {
   int num = cmd->_numberOfSimpleCommands;
   int in_fd = STDIN_FILENO, fd[2];

   // Handle input redirection for the first command
   if (cmd->_inputFile) {
      in_fd = open(cmd->_inputFile, O_RDONLY);
      if (in_fd < 0) {
         perror("input redirection failed");
         return;
      }
   }

   for (int i = 0; i < num; i++) {
      // Create a pipe for all but the last command
      if (i < num - 1) {
         if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
         }
      }

      pid_t pid = fork();
      if (pid == 0) {
         // Child process

         // Redirect input
      dup2(in_fd, STDIN_FILENO);
if (in_fd != STDIN_FILENO) close(in_fd);

         // If not last command, redirect stdout to pipe
         if (i < num - 1) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
         }
         // Handle output redirection for last command
         else if (cmd->_outFile) {
            int out_fd = open(cmd->_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) { perror("output redirection"); exit(1); }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
         }

         // Error redirection (optional)
         if (cmd->_errFile) {
            int err_fd = open(cmd->_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(err_fd, STDERR_FILENO);
            close(err_fd);
         }

         // Execute
         execvp(cmd->_simpleCommands[i]._arguments[0],
                cmd->_simpleCommands[i]._arguments);
         perror("execvp failed");
         exit(1);
      } 
      else if (pid > 0) {
         // Parent process
         if (i < num - 1) {
            close(fd[1]);       // close write end
            if (in_fd != STDIN_FILENO) close(in_fd); // close previous input fd
            in_fd = fd[0];      // next command reads from this pipe
         } else {
            // Last command: no need to keep pipe open
            if (in_fd != STDIN_FILENO) close(in_fd);
         }
      }

      else {
         perror("fork failed");
         exit(1);
      }
   }

   // Wait for all unless background
   if (!cmd->_background) {
      for (int i = 0; i < num; i++) {
         wait(NULL);
      }
   } else {
      printf("[background]\n");
   }
}




int main() {
   char line[1024];
      Command neofetch=parseCommand("neofetch");
      executeCommand(&neofetch);

   while (1) {
      printf("csh >> ");
      fflush(stdout);

      if (!fgets(line, sizeof(line), stdin)) break;

      // Built-in exit
      if (strncmp(line, "exit", 4) == 0) break;

      Command cmd = parseCommand(line);
      executeCommand(&cmd);
   }

   return 0;
}

