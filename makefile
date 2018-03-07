
SRCS=$(wildcard src/*.cpp) $(wildcard src/**/*.cpp)
OBJS=$(SRCS:.cpp=.o)

EXEC=lomda

CXXFLAGS=-Iinclude -O3 -lreadline

$(EXEC): all

all: $(OBJS)
	g++ -o $(EXEC) $^ $(CXXFLAGS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(EXEC)

re: fclean all
