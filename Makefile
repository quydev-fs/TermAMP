CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I/data/data/com.termux/files/usr/include
LDFLAGS = -L/data/data/com.termux/files/usr/lib -lX11 -lmpg123 -lpulse -lpulse-simple -lpthread

TARGET = TermuxMusic95
SRCS = main.cpp GUI.cpp AudioEngine.cpp FFT.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
