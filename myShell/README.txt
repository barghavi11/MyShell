
----------------------------------------- MY MINI SHELL ---------------------------------------------

Authors: Barghavi Gopinath (bg586) and Upasana Patel (up62)

WHAT IT IS:
-----------
mysh.c is my own mini shell that has an interactive and batch mode
interactive mode: an interactive shell that executes commands as you input them.
to run:
    make
    ./mysh
batch mode: a mode that executes commands located in a shell file, inputted as an argument. 
to run:
    make 
    ./mysh test.sh 


FUNCTIONALITY:
--------------
built-in commands: cd, which, pwd, and exit 
other commands: the path of other commands was found and executed using execv()

pipes: commands can be piped together 

tokens that contain "/" will be taken as a path and executed.

wildcards: wildcard tokens will be expanded and then handled accordingly. 
    e.g *.txt will be expanded to find all files that end in .txt

redirections: redirections "<" ">" will change stdinput or stdoutput accordingly.

conditionals: conditionals "then" and "else" will execute commands depending on the exit status 
of the previously executed command. 

TEST CASES:
----------
some example test cases are included in the test.sh batch file. these are:

touch barghavi.txt
ls > file.txt
ls | grep my
grep world < input.txt > out.txt 
echo test/*.txt
ls 
else echo do not print
gre hi 
else echo print 
exit byeee
    
