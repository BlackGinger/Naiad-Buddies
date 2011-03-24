
all: 
	make -f geo2emp.make 
	make -f emp2geo.make 

install:
	mkdir -p ${NAIAD_PATH}/buddies/houdini/bin
	cp bin/geo2emp ${NAIAD_PATH}/buddies/houdini/bin
	cp bin/emp2geo ${NAIAD_PATH}/buddies/houdini/bin

clean:
	make -f geo2emp.make clean
	make -f emp2geo.make clean

