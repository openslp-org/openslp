#
# Makefile.am
#   Automake File
#     The linked in library libcommonslpd, which is from ../common
#     containing exacts functions which are required.
#

sbin_PROGRAMS	= slpd
INCLUDES	= -I$(top_srcdir)/common -I$(srcdir)

slpd_SOURCES	= slpd_socket.c \
    slpd_database.c \
    slpd_main.c     \
    slpd_process.c  \
    slpd_cmdline.c  \
    slpd_property.c \
    slpd_regfile.c  \
    slpd_log.c      \
    slpd_knownda.c  
slpd_LDADD	=  ../common/libcommonslpd.la



