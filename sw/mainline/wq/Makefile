LIB_WQ_NAME	=libwq.a
SRCS		=$(wildcard *.c)
OBJS		=$(SRCS:.c=.o)

$(LIB_WQ_NAME) : $(OBJS)
	$(AR) rcv $@ $(OBJS)
	$(RANLIB) $@


clean:
	rm -f *.o
	rm -f $(LIB_WQ_NAME)
