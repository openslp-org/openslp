noinst_LTLIBRARIES	= libcommonslpd.la libcommonlibslp.la

if ENABLE_SLPv1
slp_v1message_SRCS = slp_v1message.c slp_utf8.c
endif

libcommonlibslp_la_SOURCES = \
	slp_buffer.c	\
	slp_message.c	\
	$(slp_v1message_SRCS) \
	slp_property.c	\
	slp_linkedlist.c\
	slp_xid.c	\
	slp_network.c	\
	slp_da.c	\
	slp_compare.c

libcommonslpd_la_SOURCES = \
	slp_compare.c	\
	slp_logfile.c	\
	slp_buffer.c	\
	slp_message.c	\
	$(slp_v1message_SRCS) \
	slp_property.c	\
	slp_linkedlist.c \
	slp_da.c

noinst_HEADERS = \
	slp_buffer.h	\
	slp_da.h	\
	slp_logfile.h	\
	slp_network.h	\
	slp_xid.h	\
	slp_compare.h	\
	slp_linkedlist.h\
	slp_message.h	\
	slp_v1message.h \
	slp_property.h
