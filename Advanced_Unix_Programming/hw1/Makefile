hw1: main.c
	gcc -static -o hw1 main.c

test: main
	mkdir -p /tmp/tmproot/bin
	cp hw1 /tmp/tmproot/bin/hw1
	sudo chmod 6755 /tmp/tmproot/bin/hw1

clean:
	rm hw1
