#
# Makefile.am
#   Automake File
#     The linked in library libcommonslpd, which is from ../common
#     containing exacts functions which are required.
#

EXTRA_DIST = slpd_win32.h slpd_win32.c
sbin_PROGRAMS = slpd
INCLUDES      = -I$(top_srcdir)/common -I$(top_srcdir)/libslpattr \
                -I$(srcdir) -I$(top_srcdir)/libslp \
                -DETCDIR=\"$(sysconfdir)\"  -DVARDIR=\"$(localstatedir)\"

if ENABLE_PREDICATES
slp_predicate_SRCS = slpd_predicate.h slpd_predicate.c
endif

if ENABLE_SLPv1
slpd_v1process_SRCS = slpd_v1process.c
endif

slpd_SOURCES	= \
$(slp_predicate_SRCS) \
$(slpd_v1process_SRCS) \
slpd_log.c \
slpd_socket.c \
slpd_database.c \
slpd_main.c \
slpd_process.c \
slpd_cmdline.c \
slpd_property.c \
slpd_regfile.c \
slpd_knownda.c \
slpd_incoming.c \
slpd_outgoing.c \
slpd.h \
slpd_knownda.h \
slpd_process.h \
slpd_unistd.h \
slpd_cmdline.h \
slpd_log.h \
slpd_property.h \
slpd_database.h \
slpd_outgoing.h \
slpd_regfile.h \
slpd_incoming.h \
slpd_socket.h
    
slpd_LDADD      =  ../common/libcommonslpd.la \
             	   ../libslpattr/libslpattr.la
