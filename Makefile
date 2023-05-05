OUTPUTDIR := bin/

CFLAGS := -std=c++14 -fvisibility=hidden -lpthread

ifeq (,$(CONFIGURATION))
	CONFIGURATION := release
endif

ifeq (debug,$(CONFIGURATION))
CFLAGS += -g
else
CFLAGS += -O3 -fopenmp
endif

SOURCES := *.cpp */*.cpp
HEADERS := *.h */*.h

TARGETBIN := maxflow-$(CONFIGURATION)

.SUFFIXES:
.PHONY: all clean

all: $(TARGETBIN)

$(TARGETBIN): $(SOURCES) $(HEADERS)
	$(CXX) -o $@ $(CFLAGS) $(SOURCES) 

format:
	clang-format -i Dinic\'s/*.cpp Ford\ Fulkerson/*.cpp ./*.cpp Dinic\'s/*.h Ford\ Fulkerson/*.h ./*.h GraphLabLite/*.h PageRank/*.h PushRelabel/*.h

clean:
	rm -rf ./maxflow-$(CONFIGURATION)

# check:	default
# 	./checker.pl