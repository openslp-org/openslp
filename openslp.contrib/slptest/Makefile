#Makefile for openslp test suite

#Set this to the root of the sopenslp source tree
#################################################
SLPROOT=../../openslp
#################################################

TESTROOT=.
SUBDIRS=FINDATTRS FINDSRVS FINDSRVTYPES REG DEREG
default: $(SUBDIRS)

PARSESRVURL: $(TESTROOT)/slp_parsesrvurl/Makefile
	@echo
	@echo
	@echo "****** Making slp_findsrvs *****************"
	$(MAKE) -C $(TESTROOT)/slp_parsesrvurl	

FINDSRVS: $(TESTROOT)/slp_findsrvs/Makefile
	@echo
	@echo
	@echo "****** Making slp_findsrvs *****************"
	$(MAKE) -C $(TESTROOT)/slp_findsrvs

FINDSRVTYPES: $(TESTROOT)/slp_findsrvtypes/Makefile
	@echo
	@echo
	@echo "****** Making slp_findsrvtypess ************"
	$(MAKE) -C $(TESTROOT)/slp_findsrvtypes

FINDATTRS: $(TESTROOT)/slp_findattrs/Makefile
	@echo
	@echo
	@echo "****** Making slp_findattrs ****************"
	$(MAKE) -C $(TESTROOT)/slp_findattrs
	
REG: $(TESTROOT)/slp_reg/Makefile
	@echo
	@echo
	@echo "****** Making slp_reg ***** ****************"
	$(MAKE) -C $(TESTROOT)/slp_reg
	
DEREG: $(TESTROOT)/slp_dereg/Makefile
	@echo
	@echo
	@echo "****** Making slp_dereg ********************"
	$(MAKE) -C $(TESTROOT)/slp_dereg
	
clean:
	-rm -rf */*.o
	-rm -rf slp_findsrvtypes/slp_findsrvtypes
	-rm -rf slp_findattrs/slp_findattrs
	-rm -rf slp_findsrvs/slp_findsrvs
	-rm -rf slp_reg/slp_reg
	-rm -rf slp_dereg/slp_dereg
	
setup: 
	cp /etc/slp.conf /etc/slp.conf~
	cp /etc/slp.reg /etc/slp.reg~
	cp $(TESTROOT)/slp.conf /etc/slp.conf
	cp $(TESTROOT)/slp.reg  /etc/slp.reg
	-killall -TERM slpd
	-sleep 2
	/usr/sbin/slpd
	
test:
	-./slp_findsrvtypes/slp_findsrvtypes
	-./slp_findsrvs/slp_findsrvs
	-./slp_findattrs/slp_findattrs
	-./slp_reg/slp_reg
	-./slp_dereg/slp_dereg
	@echo 
	@echo "****** Testing ParseSrvUrl ******************"
	-@PSU=`./slp_parsesrvurl/slp_parsesrvurl < ./slp_parsesrvurl/test.in | diff ./slp_parsesrvurl/expected.out - | grep -c ".*"`; \
	if [ $$PSU > 0 ]; then \
		echo "Result:   **FAILED**"; \
		echo "Differences: (see ./slp_parsesrvurl/README)"; \
		./slp_parsesrvurl/slp_parsesrvurl < ./slp_parsesrvurl/test.in | diff ./slp_parsesrvurl/expected.out - ; \
	else \
		echo "Result:     PASSED"; \
	fi

	
