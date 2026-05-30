CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -I./src
TARGET   = volleyball_analysis
SRCDIR   = src

SRCS = $(SRCDIR)/main.cpp \
       $(SRCDIR)/User.cpp \
       $(SRCDIR)/VideoFile.cpp \
       $(SRCDIR)/MatchEvent.cpp \
       $(SRCDIR)/Statistics.cpp \
       $(SRCDIR)/AnalysisProcessor.cpp \
       $(SRCDIR)/JSONReader.cpp \
       $(SRCDIR)/Dashboard.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(SRCDIR)/*.o $(TARGET) match_report.txt

.PHONY: all clean