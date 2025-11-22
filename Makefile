CC = gcc
WINDRES = windres
TARGET = Pong.exe
SRCS = main.c win_wrapper.c
OBJS = $(SRCS:.c=.o)
RC_FILE = resource.rc
RC_OBJ = resource.res
CFLAGS = -O2 -Wall -Wno-missing-braces -std=c99
LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -ldwmapi -mwindows -static

all: build clean

build: $(OBJS) $(RC_OBJ)
	$(CC) -o $(TARGET) $(OBJS) $(RC_OBJ) $(LDFLAGS)
	@echo [SUCCESS] $(TARGET) created successfully!

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RC_OBJ): $(RC_FILE)
	$(WINDRES) $(RC_FILE) -O coff -o $(RC_OBJ)

clean:
	del /Q $(OBJS) $(RC_OBJ) 2>nul
	@echo [CLEANING] Temp files removed.