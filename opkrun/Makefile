PRGNAME     = opkrun

# define regarding OS, which compiler to use
CC			= gcc
CXX			= g++

# change compilation / linking flag options

F_OPTS		= -DHAVE_LIBOPK -DPLATFORM=\"bittboy\" -D_CARD_ROOT=\"/media\" -I../libopk -DTARGET_MIYOO -DUSE_GZIP -DUSE_LZO -I/usr/include/SDL
CC_OPTS		= -O2 -flto -DLOG_LEVEL=4 -I../libopk $(F_OPTS)
CFLAGS		= -I$(SDL_INCLUDE) $(CC_OPTS)
CXXFLAGS	= $(CFLAGS) 
LDFLAGS     = -flto -Wl,--start-group -lc -lm -lstdc++ -lSDL -lasound -lpng -lSDL_ttf -lfreetype -lbz2 -lpng -lSDL_ttf -llzo2 -lz -Wl,--end-group -s

# Files to be compiled
SRCDIR    = . ../libopk
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP   = $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS     = $(OBJ_C) $(OBJ_CP)

# Rules to make executable
$(PRGNAME)$(EXESUFFIX): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_CP) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
