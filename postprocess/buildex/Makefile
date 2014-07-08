#make main definitions

CC=gcc
CPP=g++
CPPFLAGS=
CFLAGS=

CPPSOURCES :=
CSOURCES :=
OBJS :=

SOURCE_DIR:=src
INCLUDE_DIR:=include
OBJ_DIR:=obj
EXEC:=reverse

CPPSOURCES += reverse.cpp
CPPSOURCES += fileparser.cpp
CPPSOURCES += canonicalize.cpp

OBJS += $(addprefix $(OBJ_DIR)/, $(CPPSOURCES:.cpp=.o))
OBJS += $(addprefix $(OBJ_DIR)/, $(CSOURCES:.c=.o))
CPPSOURCES := $(addprefix $(SOURCE_DIR)/,$(CPPSOURCES))
CSOURCES := $(addprefix $(SOURCE_DIR)/,$(CSOURCES))

#make rules

#all : $(EXEC)
	
$(EXEC) : $(OBJS)
	$(CPP) -I$(INCLUDE_DIR) -o $@ $(OBJS)  

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/%.c	
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<
	
$(OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cpp	
	$(CPP) $(CPPFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

