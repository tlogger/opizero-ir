/*
g++ irsend.cpp -o irsend -lwiringPi -lpthread
*/

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <vector>

#define FREQ		38000		// Hz
#define WRITE_PIN 	0

int unit = 26;          
int duty_high = 9;      
int duty_low = 17;      

using namespace std;

typedef struct {
	char keyname[256]; 	// remote controller's key name
	int sp1, sp2;				// start pulse (us)
	int tH;							// lead pulse time (us)
	int t1;							// pulse high time (us)
	int t0;							// pulse low time (us)
	int bits;						// how many bits
	unsigned int data;	// 32bit code
	int ep1, ep2;				// end pulse (us) -- optional
} rkey_t;

void init()
{
	unit = int(1000000.0 / FREQ); 
	duty_high = int(1000000.0 / FREQ / 3.0 + 0.5);		// duty ratio = 1:3
	duty_low = unit - duty_high;
}

void pulse(int on_time, int off_time)
{
		// high
    for (int i = 0; i < (on_time / unit); i++) {
        digitalWrite(WRITE_PIN, 1); //high
        delayMicroseconds(duty_high);
        
        digitalWrite(WRITE_PIN, 0); //low
        delayMicroseconds(duty_low);
    }
    
		// low
    digitalWrite(WRITE_PIN, 0);
    delayMicroseconds(off_time);
}

vector<rkey_t> rkeys;

bool sendCode(rkey_t& k)
{
	printf("key=%s, code=%08X (%d bits)\n", k.keyname, k.data, k.bits);
	pulse(k.sp1, k.sp2);
	for (int i = k.bits - 1; i >= 0; i--) {
		if ((k.data & (1<<i)) != 0) {
			pulse(k.tH, k.t1);
		}
		else {
			pulse(k.tH, k.t0);
		}
	}
	pulse(k.ep1, k.ep2);
}

bool sendKey(char* key)
{
	rkey_t k;
	
	for (int i = 0; i < rkeys.size(); i++) {
		k = rkeys[i];
		if (strcmp(key, k.keyname) == 0) {
			return sendCode(k);
		}
	}
	
	return false;
}

int main(int argc, char *argv[])
{
    FILE *f;
    
    if (argc < 3) {
    	printf("usage: irsend filename keys...\n");
    	return 0;
    }
    
    if ((f = fopen(argv[1], "r")) == NULL){
        printf("can't open file: '%s'\n", argv[1]);
        exit(1);
    }
	
		while (!feof(f)) {
			rkey_t k;
			fscanf(f, "%s%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %08X%*c %d%*c %d\n", k.keyname, &k.sp1, &k.sp2, &k.tH, &k.t1, &k.t0, &k.bits, &k.data, &k.ep1, &k.ep2);
			k.keyname[strlen(k.keyname)-1] = '\0';
			rkeys.push_back(k);
		}
		
    fclose(f);

    if(wiringPiSetup() == -1){
        printf("error wiringPi setup\n");
        exit(1);
    }

    pinMode(WRITE_PIN, OUTPUT);
    printf("output pin: %d (wiringpi)\n", WRITE_PIN);
    
		init();
		
		for (int i = 2; i < argc; i++) {
			sendKey(argv[i]);
			delayMicroseconds(1000000);
		}

    return 0;
}
