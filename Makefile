miniweb: miniweb.o queue.o base64.o
	gcc miniweb.o queue.o base64.o -pthread -o miniweb

miniweb.o: miniweb.c queue.h
	gcc miniweb.c -c -g -pthread

queue.o: queue.c queue.h
	gcc queue.c -c -g -pthread

base64.o: base64.c base64.h
	gcc base64.c -c -g