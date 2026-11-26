# SHELL TAKE-HOME ASSIGNMENT

First compile, then run: `make && ./shell`.  

Subsequent runs: `./shell`.  

# COMMANDS:

## ls

`ls` - lists contents of current working directory.
`ls <PATH>` - lists contents of given directory in the path.
`ls -<FLAG> <PATH>` - lists contents of given directory in the path, with flag.

### ls flags

Any combination of the supported flags works, thanks to the getopt function.  

Supported flags:

`-a` - lists all contents of directory, including hidden folders.
`-l` - lists contents of directory, row by row, with permissions, user, group, 
size and date last touched.

## cd 

`cd <PATH>` - changes current working directory to the one in the given path.  

If there is no argument given, `cd` changes the cwd to the home directory.

## pwd

`pwd` - prints current working directory

## last

`last` - prints return value of last non-builtin command

