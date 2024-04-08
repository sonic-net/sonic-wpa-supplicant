ALL=radius_lib

include ../src/build.rules


CFLAGS += -I../src
CFLAGS += -I../src/utils

LIBS = ../src/radius/libradius.a
LIBS += ../src/crypto/libcrypto.a
LIBS += ../src/utils/libutils.a

CFLAGS += -DCONFIG_SONIC_RADIUS

_OBJS_VAR := LIBS
include ../src/objs.mk

radius_lib: $(LIBS)
		echo "Building Radius lib ..."
clean: common-clean
	rm -f core *~ *.o *.d
