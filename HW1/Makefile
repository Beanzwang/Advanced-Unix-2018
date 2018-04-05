COMPILE = g++ -static -o hw1 main.cpp

main: main.cpp
	$(COMPILE)

tmp:
	mkdir -p /tmp/tmproot/bin
	cp hw1 /tmp/tmproot/bin/hw1
	sudo chmod 6755 /tmp/tmproot/bin/hw1

xinetd:
	/etc/init.d/xinetd start

telnet:
	telnet localhost 33445

nc:
	nc localhost 33445

clear:
	rm *.o
