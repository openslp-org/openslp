#
# Very Simple LIBSLP Makefile
# 
SONAME		= libslp
MAJORVER	= 0
MINORVER	= 6.6
BASE_DIR   	= .
LIBSLP_DIR   	= $(BASE_DIR)
COMMON_DIR 	= $(BASE_DIR)/../common
TEST_DIR	= $(BASE_DIR)

LIBSLP_SRC = $(LIBSLP_DIR)/libslp_dereg.c   \
             $(LIBSLP_DIR)/libslp_findscopes.c \
	     $(LIBSLP_DIR)/libslp_reg.c \
	     $(LIBSLP_DIR)/libslp_findsrvs.c \
	     $(LIBSLP_DIR)/libslp_parse.c \
	     $(LIBSLP_DIR)/libslp_property.c  \
	     $(LIBSLP_DIR)/libslp_handle.c  \
	     $(LIBSLP_DIR)/libslp_xid.c \
	     $(LIBSLP_DIR)/libslp_network.c \
	     $(LIBSLP_DIR)/libslp_thread.c \
             $(LIBSLP_DIR)/libslp_findattrs.c \
	     $(LIBSLP_DIR)/libslp_delattrs.c \
	     $(LIBSLP_DIR)/libslp_findsrvtypes.c
	      
       
COMMON_SRC = $(COMMON_DIR)/slp_buffer.c  \
	     $(COMMON_DIR)/slp_message.c \
	     $(COMMON_DIR)/slp_property.c \
             $(COMMON_DIR)/slp_linkedlist.c

TEST_SRC   = $(TEST_DIR)/LIBSLP_test.c

release: 
	gcc -Wall -O3 -fPIC -shared -Wl,-soname,$(SONAME).so -o$(SONAME).so.$(MAJORVER).$(MINORVER) -I$(COMMON_DIR) -I$(LIBSLP_DIR) $(LIBSLP_SRC) $(COMMON_SRC)

debug:
	gcc -Wall -g -shared -Wl,-soname,$(SONAME).so -o$(SONAME).so.$(MAJORVER).$(MINORVER) -DDEBUG -I$(COMMON_DIR) -I$(LIBSLP_DIR) $(LIBSLP_SRC) $(COMMON_SRC)

clean:
	rm -f libslp.so*

install: clean release
	chmod a+x install.sh
	./install.sh
