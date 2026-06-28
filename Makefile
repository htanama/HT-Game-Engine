# Example: g++ -o myapp main.cpp glad.c $(pkg-config --cflags --libs sdl2) -lGL
# or Ex: g++ -o myapp.exe main.cpp glad.c -IC:/path/to/SDL2/include -LC:/path/to/SDL2/lib -lSDL2

CC = gcc 
CXX = g++ 
CXXFLAGS = -Wall -g  
INC = -I. -I./core -I./editor \
 -I./glad/include \
 -I./glm \
 -I./imgui \
 -I$(HOME)/Local/SDL3/include

IMGUI_SOURCES = ./imgui/imgui.cpp ./imgui/imgui_draw.cpp ./imgui/imgui_tables.cpp ./imgui/imgui_widgets.cpp \
 ./imgui/backends/imgui_impl_sdl3.cpp \
 ./imgui/backends/imgui_impl_opengl3.cpp \
 ./imgui/imgui_demo.cpp

all: HT_Game_Engine

LIBS = -L$(HOME)/Local/SDL3/lib -lSDL3 -lGL

Main.o: Main.cpp    
	$(CXX) -Wall -c Main.cpp $(INC) -o Main.o

# We need to compile the glad.c separately using GCC 
glad.o: glad/src/glad.c
	$(CC) -Wall -c glad/src/glad.c $(INC) -o glad.o

# The target
HT_Game_Engine: Main.o glad.o
	$(CXX) -Wall -g $(INC) $(IMGUI_SOURCES) Main.o glad.o -o HT_Game_Engine $(LIBS)

clean:
	rm -f *.o HT_Game_Engine
