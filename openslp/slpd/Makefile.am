#
# Makefile.am
#   Automake File
#     The linked in library libcommonslpd, which is from ../common
#     containing exacts functions which are required.
#

sbin_PROGRAMS = slpd
INCLUDES      = -I$(top_srcdir)/common -I$(top_srcdir)/libslpattr \
                -I$(srcdir) -I$(top_srcdir)/libslp \
                -DETCDIR=\"$(sysconfdir)\"  -DVARDIR=\"$(localstatedir)\"

if ENABLE_SLPv1
slpd_v1process_SRCS = slpd_v1process.c
endif

slpd_SOURCES	= slpd_socket.c \
    slpd_database.c \
    slpd_main.c     \
    slpd_process.c  \
    $(slpd_v1process_SRCS) \
    slpd_cmdline.c  \
    slpd_property.c \
    slpd_regfile.c  \
    slpd_log.c      \
    slpd_knownda.c  \
    slpd_incoming.c \
    slpd_outgoing.c \
    slpd_predicate.c

slpd_LDADD      =  ../common/libcommonslpd.la ../libslpattr/libslpattr.la
