CFLAGS += -DCONFIG_IPV6
CFLAGS += -fPIC
CFLAGS += -DCONFIG_SONIC_RADIUS
CFLAGS += -DCONFIG_SONIC_RADIUS_MAB

LIB_OBJS= \
	radius.o \
	radius_client.o \
	radius_das.o \
	radius_server.o
LIB_OBJS += radius_attr_parse.o

include ../lib.rules
