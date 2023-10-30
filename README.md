# LinuxShellInterface
Linux Shell Interface - CS361: Systems Programming

Live Demo: https://replit.com/@fabiantorrestec/LinuxShellInterface#README.md

Example: ![image](https://github.com/fabiantorrestech/LinuxShellInterface/assets/46858378/98de992d-9b9e-414f-911d-14c14a4ae200)

- Wrote Shell Interface using Linux Libraries and Systems Calls. 
- Supports 2 feature piping, waiting, input & output redirection. 
- Demonstrates understanding of parent and child processes utilizing posix_spawn and fork() methods. (implemented using posix_spawn() child creation)
- Implemented in C.

----------------
To Compile:

Type these into the terminal to compile each one:

    make spawnshell
    
    make forkshell
    
----------------
To Run: 

For ForkShell -- 

    ./forkshell

For SpawnShell --

    ./spawnshell

----------------
Normally on most Linux terminals, you can Ctrl+C or Ctrl+X to quit out of any terminal application, but since we want to catch that input and alert the user of it in this project, the terminal will not quit when those commands are invoked. Instead it will repeat back "[caught sigint](https://man7.org/linux/man-pages/man7/signal.7.html)" to alert that we have caught that input (interrupt from keyboard -- termination signal).

To quit the application, you can simply type "quit" or "exit".
