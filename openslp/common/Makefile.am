noinst_LTLIBRARIES	= libcommonslpd.la libcommonlibslp.la
EXTRA_DIST = 

if ENABLE_SLPv1
slp_v1message_SRCS = slp_v1message.c slp_utf8.c
endif

if ENABLE_SLPv2_SECURITY
slp_security_SRCS = slp_auth.c slp_crypto.c slp_spi.c
endif

libcommonlibslp_la_SOURCES = \
	slp_compare.c \
	slp_buffer.c \
	slp_message.c \
	slp_property.c \
	slp_linkedlist.c \
	slp_xid.c \
	slp_network.c \
	slp_database.c \
	slp_xmalloc.c \
	slp_xcast.c \
	slp_iface.c \
	slp_parse.c \
	slp_pid.c \
	slp_dhcp.c \
	$(slp_v1message_SRCS) \
	$(slp_security_SRCS)

libcommonslpd_la_SOURCES = \
	slp_compare.c \
	slp_buffer.c \
	slp_message.c \
	slp_property.c \
	slp_linkedlist.c \
	slp_xid.c \
	slp_database.c \
	slp_xmalloc.c \
	slp_parse.c \
	slp_iface.c \
	slp_net.c \
	slp_pid.c \
	slp_attr_y.y \
	slp_attr_l.l \
	slp_filter_y.y \
	slp_filter_l.l \
	slp_predicate.c \
	$(slp_v1message_SRCS) \
	$(slp_security_SRCS)
	

noinst_HEADERS = \
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
	slp_utf8.h \
	slp_xcast.h \
	slp_iface.h \
	slp_parse.h \
	slp_net.h \
	slp_pid.h \
	slp_dhcp.h
	
AM_YFLAGS = -d
