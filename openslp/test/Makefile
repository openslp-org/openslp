MAKE	= make

all:
	$(MAKE) -C SLPOpen
	$(MAKE) -C SLPFindSrvs
	$(MAKE) -C SLPReg
	$(MAKE) -C SLPDereg
	$(MAKE) -C SLPFindAttrs
	$(MAKE) -C SLPParseSrvURL

test:
	$(MAKE) -s -C SLPOpen test
	$(MAKE) -s -C SLPFindSrvs test
	$(MAKE) -s -C SLPReg test
	$(MAKE) -C SLPDereg test
	$(MAKE) -C SLPFindAttrs test
	$(MAKE) -C SLPParseSrvURL test

clean:
	$(MAKE) -C SLPOpen clean
	$(MAKE) -C SLPFindSrvs clean
	$(MAKE) -C SLPReg clean
	$(MAKE) -C SLPDereg clean
	$(MAKE) -C SLPFindAttrs clean 
	$(MAKE) -C SLPParseSrvURL clean
