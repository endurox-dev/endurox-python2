all:
	NDRX_HOME=/usr python setup.py build --force
        
install:
	NDRX_HOME=/usr python setup.py install
