
CC = g++
LD = ld
LDFLAGS  = -L/usr/local/lib -L/opt/lampp/lib -lmysqlclient -lpthread -L/usr/local/lib -levent
INCLUDES = -I. -I/opt/lampp/include
CPPFLAGS = -O2 -I. -I/opt/lampp/include

COMPILE = $(CC) $(INCLUDES) $(CPPFLAGS)
OBJ = VspdCToMySQL.o tableAgent.o appleServer.o

.c.o:
	$(CC) $< $(INCLUDES) $(CPPFLAGS)
	
appleserver: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

clean:
	rm -rf *.o
	rm -rf appleserver
