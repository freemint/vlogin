# setup crosscompile environment
#
# include ~/projects/mint/freemint/CONFIGVARS
# CFLAGS += -I../../gemdev/lib/windom.mt/include $(GENERAL)

#CFLAGS += -ggdb -O0 -Wall
CFLAGS += -g -O2 -m68020-60 -Wall

OBJS = tiny_aes.o vdi_login.o list.o signals.o verify.o md5crypt.o logon.o\
conf.o after_login.o vdi_it.o moose.o debug.o

LFLAGS = -lgem

TARGET = vlogin

all: $(TARGET)

$(TARGET): $(OBJS) 
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LFLAGS)

strip:
	strip $(TARGET)

clean:
	rm -rf $(TARGET) *.o

install:
	cp $(TARGET) /sbin/
