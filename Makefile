MAIN = GLXBasics
CC = gcc
GLEW = extern/src/glew
CFLAGS = -Iextern/include -Wall -Wextra -pedantic -std=c99
LIBS = -lX11 -lGL -lm

all : $(MAIN)

$(MAIN).o : $(MAIN).c
$(GLEW).o : $(GLEW).c

$(MAIN) : $(MAIN).o $(GLEW).o
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN).c $(GLEW).c $(LIBS)

clean:
	rm -f $(MAIN).o $(GLEW).o

fclean:clean
	rm -f $(MAIN)
