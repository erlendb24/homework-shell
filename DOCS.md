# Homework shell

Notes for wanted behaviour of the program

## Flow for user:

User runs ./shell -> opens the interactive shell -> 
types command 'ls -al' -> the program prints the user directories

## Program flow:

Program start ->  
loop while waiting for user input ->  
fgets is used to capture user input and its sent to an eval function ->  
user input is tokenized ->  
switch statement on first token (command) ->  
if command is ls, cd or pwd -> BRANCH 1 ->  
else -> BRANCH 2  

### BRANCH 1:  
if command is ls -> LS -> BRANCH 1 END ->  
if command is cd -> CD -> --||-- ->  
if command is pwd -> PWD -> --||-- ->  

### BRANCH 1 END:  
returns to loop and waits for next command  

### BRANCH 2:  
the process is forked ->  
if process id != 0 -> PARENT  
if process id == 0 -> CHILD  

### PARENT:  
waits for child process to terminate -> BRANCH 2 END  

### CHILD:  
calls execvp with arguments given ->  
process is killed  

### BRANCH 2 END  
returns to loop and waits for next command  

### PWD:  
Prints current working directory  

### CD:  
Changed current directory to the selected directory  
if there is no directory in relative path that matches, it fails  

### LS:  
Prints directories  
if flag -a is activated, prints "hidden" folders  
if flag -l is activated, prints as a vertical list, instead of a block  

use opendir on current dir, unless another dir is specified - 

