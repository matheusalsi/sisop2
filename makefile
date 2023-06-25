PROJECT_NAME = wakeonlan.app

COMPILER = g++

FLAGS = -Wall --std=c++11 -g 

LIBS = -pthread

MAIN = app/main.cpp

SRCS = $(shell find ./app -iname "*.cpp")

all: $(PROJECT_NAME)
	@echo Project compiled

$(PROJECT_NAME): $(MAIN)
	$(COMPILER) $(FLAGS) -o $(PROJECT_NAME) $(SRCS) $(LIBS)
	
runclient: $(PROJECT_NAME)
	./$(PROJECT_NAME)

runmanager: $(PROJECT_NAME)
	./$(PROJECT_NAME) manager

clean:
	rm -rf *.o main $(PROJECT_NAME)

manager: clean all runmanager

client: clean all runclient