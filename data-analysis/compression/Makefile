#  Maintenance targets:
#
#
#    all         - (default target) make sure everything's compiled
#    clean       - clean out all compiled object and executable files
#

# Executables to built using "make all"
EXECUTABLES = compress

#
#  The following is a compromise. You MUST list all your .h files here.
#  If any .h file changes, all .c files will be recompiled. To do better,
#  we could make an explicit target for each .o, naming only the .h
#  files it really uses.
#
# Add your own .h files to the right side of the assingment below.
INCLUDES = *.h

# Do all C compies with gcc (at home you could try clang)
CC = gcc

# Updating include path to use current directory
IFLAGS = -I. 

# the next line enables you to compile and link against course software
CFLAGS =  -g -std=c99 -Wall -Wextra -Werror -Wfatal-errors -pedantic $(IFLAGS)

# 
#    'make all' will build all executables
#
#    Note that "all" is the default target that make will build
#    if nothing is specifically requested
#
all: $(EXECUTABLES)

# 
#    'make clean' will remove all object and executable files
#
clean:
	rm -f $(EXECUTABLES) *.o


# 
#    To get any .o, compile the corresponding .c
#
%.o:%.c $(INCLUDES) 
	$(CC) $(CFLAGS) -c $<

#
# Individual executables
#
#    Each executable depends on one or more .o files.
#    Those .o files are linked together to build the corresponding
#    executable.
#
compress: compress.o array.o
	$(CC) $(LDFLAGS) -o compress.o array.c $(LDLIBS)

#
# Other Shortcuts worth nothing
# $@ takes the name of the build rule and inserts it into the command
# $^ inserts the relocatable object file names into the command
#
