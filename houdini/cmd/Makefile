
all: 
	make -f geo2emp.make 
	make -f emp2geo.make 
	make -f rop_naiadcamera.make

install:
	mkdir -p ${NAIAD_PATH}/buddies/houdini/bin
	mkdir -p ${NAIAD_PATH}/buddies/houdini/dso
	cp bin/geo2emp ${NAIAD_PATH}/buddies/houdini/bin
	cp bin/emp2geo ${NAIAD_PATH}/buddies/houdini/bin
	cp bin/ROP_NaiadCamera.so ${NAIAD_PATH}/buddies/houdini/dso
	#make -f rop_naiadcamera.make install

clean:
	make -f geo2emp.make clean
	make -f emp2geo.make clean
	make -f rop_naiadcamera.make clean

