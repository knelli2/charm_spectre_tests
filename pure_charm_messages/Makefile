-include ../../../common.mk
CHARMC	= ../../../../bin/charmc $(OPTS)

all: pure_charm_messages

pure_charm_messages: pure_charm_messages.decl.h pure_charm_messages.cpp
	$(CHARMC) pure_charm_messages.cpp -o pure_charm_messages 

pure_charm_messages.decl.h: pure_charm_messages.ci
	$(CHARMC) pure_charm_messages.ci

test: pure_charm_messages
	$(call run, +n2 +p2 ./pure_charm_messages)

clean:
	rm -rf *.decl.h *.def.h pure_charm_messages charmrun
