RANLIB				= @RANLIB@
noinst_LIBRARIES	= libcommonslpd.a libcommonlibslp.a
CFLAGS				+= -Wall -O3

libcommonlibslp_a_SOURCES = \
	slp_buffer.c		\
	slp_message.c		\
	slp_property.c		\
	slp_linkedlist.c	\
	slp_xid.c			\
	slp_network.c		\
	slp_da.c			\
	slp_compare.c

libcommonslpd_a_SOURCES = \
	slp_compare.c		\
	slp_logfile.c		\
	slp_buffer.c		\
	slp_message.c		\
	slp_property.c		\
	slp_linkedlist.c

noinst_HEADERS = \
	slp_buffer.h		\
	slp_da.h			\
	slp_logfile.h		\
	slp_network.h		\
	slp_xid.h			\
	slp_compare.h		\
	slp_linkedlist.h	\
	slp_message.h		\
	slp_property.h
