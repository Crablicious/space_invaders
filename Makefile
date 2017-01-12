#OBJS specifies which files to compile as part of the project 
OBJS = space_invaders.c 
#CC specifies which compiler we're using 
CC = gcc 

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

#COMPILER_FLAGS specifies the additional compilation options we're using 
COMPILER_FLAGS = $(SDL_CFLAGS) -Wall -std=c99 -ggdb	 
#LINKER_FLAGS specifies the libraries we're linking against 
LINKER_FLAGS = $(SDL_LDFLAGS) -lSDL2_image -lSDL2_mixer -lSDL2_ttf
#OBJ_NAME specifies the name of our exectuable 
OBJ_NAME = run_space_invaders 

#This is the target that compiles our executable 
all : $(OBJS) 
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)


.PHONY : clean
clean :
	-rm -f $(OBJ_NAME) *~ *.o

#test: sdl_test2.c
#	gcc -o myprogram sdl_test2.c `sdl2-config --cflags --libs` -lSDL2 -lSDL2_image -std=c99


