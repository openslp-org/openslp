#if you're building on Irix, exchange commented and uncommented lines
#noinst_LTLIBRARIES	= libcommonslpd.a libcommonlibslp.a
noinst_LTLIBRARIES	= libcommonslpd.la libcommonlibslp.la


EXTRA_DIST = 

if ENABLE_SLPv1
slp_v1message_SRCS = slp_v1message.c slp_utf8.c
endif

if ENABLE_SLPv2_SECURITY
slp_security_SRCS = slp_auth.c slp_crypto.c slp_spi.c
endif

#if you're building on Irix, exchange commented and uncommented lines
#libcommonlibslp_a_SOURCES = \
#
libcommonlibslp_la_SOURCES = \
	slp_dhcp.c\
	slp_iface.c\
	slp_net.c\
	slp_parse.c\
	slp_pid.c\
	slp_xcast.c\
        slp_compare.c \
	slp_buffer.c \
	slp_message.c \
	slp_property.c \
	slp_linkedlist.c \
	slp_xid.c \
	slp_network.c \
	slp_database.c \
	slp_xmalloc.c \
	$(slp_v1message_SRCS) \
	$(slp_security_SRCS)

#if you're building on Irix, exchange commented and uncommented lines
#libcommonslpd_a_SOURCES = \
#
libcommonslpd_la_SOURCES = \
        slp_compare.c \
	slp_dhcp.c\
	slp_iface.c\
	slp_net.c\
	slp_parse.c\
	slp_pid.c\
	slp_buffer.c \
	slp_message.c \
	slp_property.c \
	slp_linkedlist.c \
	slp_xid.c \
	slp_database.c \
	slp_xmalloc.c \
	$(slp_v1message_SRCS) \
	$(slp_security_SRCS)

noinst_HEADERS = \
	slp_attr.h\
	slp_dhcp.h\
	slp_filter.h\
	slp_iface.h\
	slp_net.h\
	slp_parse.h\
	slp_pid.h\
	slp_predicate.h\
	slp_win32.h\
	slp_xcast.h\
	slp_buffer.h \
	slp_network.h \
	slp_xid.h \
	slp_compare.h \
	slp_linkedlist.h \
	slp_database.h \
	slp_message.h \
	slp_v1message.h \
	slp_property.h \
	slp_crypto.h \
	slp_auth.h \
	slp_spi.h \
	slp_xmalloc.h \
	slp_utf8.h 
