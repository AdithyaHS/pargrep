CC	= 	gcc
CFLAGS	=	-g -Wall -Iinclude -w -lpthread
TARGET	=	pargrep
all: $(TARGET)
$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c  
clean:
	$(RM) $(TARGET)
#all:sys_info.c
#	gcc -g -Wall -o sys_info sys_info.c -Iinclude
