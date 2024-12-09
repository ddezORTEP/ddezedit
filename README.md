# ddezedit
text editor written in c++ and ncurses
# versions
there exist multiple versions of this program. These include : 
- version with vi-like commands (linux only)
- version with normal movement and commands (linux and windows)
# usage
usage of the editor in the command prompt can be described as so on linux : 
``pbedit <file name> ``
or if the program is not in your program path in shell : 
``./pbedit <file name> ``
on windows, it is the following
`` start pbedit.exe <file name>``
# compiling
This editor can be compiled with the following commands. Firstly, clone into the repository using : 
``git clone https://github.com/ddezORTEP/ddezedit ``
Then, you can compile the source code using gcc : 
`` g++ pbedit-<version>.cpp -o pbedit -lncurses``
The following command can be using on windows using MinGW : 
`` g++ pbedit-<version>.cpp -o pbedit.exe -lncurses``
# future version
I do not plan on updating this project. I don't recommend you use this as your text editor either (unless you use nano, in which case be my guest).
