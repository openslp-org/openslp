#
# Makefile.am
#   Automake File
#     The linked in library libcommonslpd, which is from ../common
#     containing exacts functions which are required.
#

DIRS 			= common
sbin_PROGRAMS	= slpd
INCLUDES		= -I../common -I../slpd
slpd_LDADD		=  $(top_srcdir)/common/libcommonslpd.a

slpd_SOURCES	= slpd_socket.c \
    slpd_database.c \
    slpd_main.c     \
    slpd_process.c  \
    slpd_cmdline.c  \
    slpd_property.c \
    slpd_regfile.c  \
    slpd_log.c

../common/libcommon.a:
