CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = .
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/C6502Test

SRC = \
C6502Test.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

LIBS = -lC6502

CPPFLAGS = \
-std=c++14 \
-I$(INC_DIR) \
-I../../C6502/include \
-I.

LFLAGS = \
-L$(LIB_DIR) \
-L../../C6502/lib \

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/C6502Test

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/C6502Test: $(OBJS) $(LIB_DIR)/libC6502.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/C6502Test $(OBJS) $(LFLAGS) $(LIBS)
