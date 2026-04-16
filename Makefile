all: #just for default target
	$(MAKE) -C src $@
%, %.o: #not working for default without target
	$(MAKE) -C src $@

.PHONY: % all

