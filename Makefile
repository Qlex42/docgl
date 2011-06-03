MAIN = GLXBasics
CC = g++
GLEW = extern/src/glew
CPPFLAGS = -Iextern/include -Wall -Wextra
LIBS = -lX11 -lGL -lm

all : $(MAIN)

$(MAIN).o : $(MAIN).cpp
$(GLEW).o : $(GLEW).c

$(MAIN) : $(MAIN).o $(GLEW).o
	$(CC) $(CPPFLAGS) -o $(MAIN) $(MAIN).cpp $(GLEW).c $(LIBS)

clean:
	rm -f $(MAIN).o $(GLEW).o

fclean:clean
	rm -f $(MAIN)
