OBJS := \
    appfilter_user.o \
    appfilter_netlink.o \
    appfilter_ubus.o \
    appfilter_config.o \
    appfilter_custom_rule.o \
    utils.o \
    main.o

EXEC := oafd

LDLIBS += \
    -lubox \
    -lubus \
    -luci \
    -ljson-c \
    -lblobmsg_json \
    -lpthread \
    -lm

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)
