#
# Very Simple SLPD Makefile
# 


BINARY		= slpd
BASE_DIR   	= .
SLPD_DIR   	= $(BASE_DIR)
COMMON_DIR 	= $(BASE_DIR)/../common

SLPD_SRC = $(SLPD_DIR)/slpd_socket.c   \
           $(SLPD_DIR)/slpd_database.c \
	   $(SLPD_DIR)/slpd_main.c     \
	   $(SLPD_DIR)/slpd_process.c  \
           $(SLPD_DIR)/slpd_cmdline.c  \
	   $(SLPD_DIR)/slpd_property.c \
	   $(SLPD_DIR)/slpd_regfile.c 
	          

COMMON_SRC = $(COMMON_DIR)/slp_compare.c  \
             $(COMMON_DIR)/slp_logfile.c \
	     $(COMMON_DIR)/slp_buffer.c  \
	     $(COMMON_DIR)/slp_message.c \
	     $(COMMON_DIR)/slp_property.c \
	     $(COMMON_DIR)/slp_linkedlist.c
	     
release:
	gcc -O3 -o $(BINARY) -Wall -I$(COMMON_DIR) -I$(SLPD_DIR) $(SLPD_SRC) $(COMMON_SRC)
	
paranoid:
	gcc -O3 -o $(BINARY) -Wall -DPARANOID -I$(COMMON_DIR) -I$(SLPD_DIR) $(SLPD_SRC) $(COMMON_SRC)	
	
debug:
	gcc -g -o $(BINARY) -Wall -DDEBUG -I$(COMMON_DIR) -I$(SLPD_DIR) $(SLPD_SRC) $(COMMON_SRC)
	
install: release
	chmod a+x install.sh
	./install.sh
	

	
