
all: 
	make -f geo2emp.make 
	make -f emp2geo.make 

clean:
	make -f geo2emp.make clean
	make -f emp2geo.make clean

