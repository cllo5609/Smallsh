# smallsh

This program imitates a shell using C language. This program has two built in commands 'cd' and 'exit'. All other commands are 
handled by creating a new process using the 'fork' function and calling execvp in the child process. Processes can be run in the
background by adding an ampersand character '&' to the end of the command line, otherwise a process is run in the foreground.
On background processes, the parent process does not wait for the child to finish executing before returning. With the foreground
process, the parent process will wait until the child has executed before continuing. The exit status or signal status is returned
and saved as a global variable. This program includes the use of special tokens including:

- "~/" will be replaced with the value of the HOME environment variable
- "$$" will be replaced with the process ID of the smallsh process
- "$?" will be replaced with the exit status of the last foreground command
- "$!" will be replaced with the process ID of the most recent background process

This program also includes input and output file redirection using "<" and ">", respectively. 
