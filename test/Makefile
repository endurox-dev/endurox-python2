all:	demo

demo:	TUXCONFIG tlog QFS server_exe

tlog:
	python rplc.py <  create_tlog | bash

QFS:
	@if test -f QFS; then (python rplc.py < delete_queues | bash); fi
	(python rplc.py < create_queues | bash ); true

server_exe:
	python rplc.py < pyserver.py.templ > pyserver.py
	chmod +x pyserver.py
	python rplc.py < recv.py.templ > recv.py
	chmod +x recv.py
	python rplc.py < send.py.templ > send.py
	chmod +x send.py
	python rplc.py < simpcl.py.templ > simpcl.py
	chmod +x simpcl.py
	python rplc.py < testclient.py.templ > testclient.py
	chmod +x testclient.py

TUXCONFIG: ubbconfig
	python rplc.py < ubbconfig | tmloadcf -y -


clean:	
		-rm -f *.a tags TAGS config.c Makefile.pre $(TARGET) sedscript
		-rm -f *.so *.sl so_locations
		-rm -f  *.pyc ULOG* stderr stdout tuxmodule.so QFS TUXFS TUXCONFIG tm*evt* 
		-rm -f testclient.py pyserver.py recv.py send.py simpcl.py 
		-ipcrm `ipcs -q | grep ${USER}|awk  '{ print "-q " $$2 }'`        
		-ipcrm `ipcs -s | grep ${USER}|awk  '{ print "-s " $$2 }'`        
		-ipcrm `ipcs -m | grep ${USER}|awk  '{ print "-m " $$2 }'`        
		-rm -f *.o *~ ULOG* core
