BIN = irsend irscan

all : $(BIN)

clean:
	rm -fr $(BIN)

irsend: irsend.cpp 
	g++ -o $@ $< -lwiringPi -lpthread 

irscan: irscan.cpp 
	g++ -o $@ $< -lwiringPi -lpthread 
	
