include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -I. -I..
CFLAGS+=-DDEBUG -g

all: libnetworking.a

#use macros BASEFILE__ if no need full srcpath in log debug file
%.o: %.c
	$(CC) $(CFLAGS) -DBASEFILE__=\"$(notdir $<)\" $< -o $@

libnetworking.a: \
	$(CURDIR)/eachtoother_comm.o \
	$(CURDIR)/channels_conf.o \
	$(CURDIR)/channels_conf_reader.o
	@ar rcs libnetworking.a eachtoother_comm.o channels_conf.o channels_conf_reader.o

clean:
	@rm -f *.a *.o 

