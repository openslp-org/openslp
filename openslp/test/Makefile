MAKE	= make
DIRS	= SLPOpen SLPFindSrvs SLPReg SLPDereg SLPFindAttrs \
			SLPParseSrvURL SLPEscape
all:
	for dir in $(DIRS); do\
		echo Making all in $$dir... ;\
		(cd $$dir; $(MAKE))|| break;\
    done

test:
	for dir in $(DIRS); do\
		echo Making all in $$dir... ;\
		(cd $$dir; $(MAKE) test)|| break;\
    done

clean:
	for dir in $(DIRS); do\
		echo Making all in $$dir... ;\
		(cd $$dir; $(MAKE) clean)|| break;\
    done
