/*
g++ irscan.cpp -o irscan -lwiringPi -lpthread -W-deprecated-declarations
*/

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <vector>

#define READ_PIN	7
#define PULSE_INTERVAL 10		// 10us
#define PULSE_MAX 		40000	// 40ms

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

vector<rkey_t> rkeys;

double getMoment()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((double)(tv.tv_sec) * 1000000 + (double)(tv.tv_usec));
}

int getTime(int status)
{
	int count = 0;
	int max = PULSE_MAX / PULSE_INTERVAL;
	double start = getMoment();
	
	while( digitalRead(READ_PIN) == status ) {
		delayMicroseconds(PULSE_INTERVAL);
		if (++count > max)
			break;
	}

	return int(getMoment() - start);
}

bool scan(char* keyname)
{
	vector<int> a;
	
	// expect 0 at first
	if(!digitalRead(READ_PIN)) {
		printf("pin should be low at first. canceled\n");
		return false;
	}
	
	// wait till it's not 0
	while( digitalRead(READ_PIN) )
		;
		
	int on, off;
	
	while( 1 ) {
		on = getTime(0);
		off = getTime(1);
		
		a.push_back(on);
		a.push_back(off);
	
		if(off > PULSE_MAX)
			break;
	}

	if (a.size() < 10) {
		printf("wrong IR code captured. cancel\n");
		return false;
	}
	
	rkey_t k;
	k.sp1 = a[0];
	k.sp2 = a[1];
	k.ep1 = a[a.size()-2];
	k.ep2 = a[a.size()-1];
	
	k.bits = (a.size() - 4) / 2;
	
	k.tH = 0;
	for (int i = 2; i < a.size()-2; i += 2) 
		k.tH += a[i];
	k.tH /= k.bits;

	int t0 = 0, t0c = 0;
	int t1 = 0, t1c = 0;
	unsigned int code = 0;
	for (int i = 2; i < a.size()-2; i += 2) {
		int on = a[i];
		int off = a[i+1];
	
		code = (code << 1);	
		if (off > k.tH) {		
			t1 += off;
			t1c++;
			code |= 1;
		}
		else {
			t0 += on;
			t0c++;
		}
	}
	
	if (t0c == 0 || t1c == 0) {
		printf("wrong IR code captured. canceled\n");
		return false;
	}
	
	k.data = code;
	k.t0 = t0 / t0c;
	k.t1 = t1 / t1c;
	memset(k.keyname, 0, sizeof(k.keyname));
	strncpy(k.keyname, keyname, sizeof(k.keyname));
	
	printf("Start[%d, %d] bit:%d, code:0x%08X, End[%d,%d], T[%d,%d,%d]\n", 
		k.sp1, k.sp2, k.bits, k.data, k.ep1, k.ep2, k.tH, k.t0, k.t1);
	
	rkeys.push_back(k);
	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: irscan filename");
		return 0;
	}

	if(wiringPiSetup() == -1){
		printf("error wiringPi setup\n");
		exit(1);
	}
	
	pinMode(READ_PIN, INPUT);
	printf("scaning pin: %d (wiringpi)\n", READ_PIN);
	
	// scan for each key
	while (1) {
		char s[256];
		printf("enter key name (Q=quit): ");
		fgets(s, sizeof(s), stdin);
	
		s[strlen(s)-1] = '\0';
		if (strcmp(s, "q") == 0)
			break;
			
		scan(s);
	}
	
	FILE *f;
	
	if((f = fopen(argv[1], "w")) == NULL){
		printf("can't open file : %s\n", argv[1]);
		exit(1);
	}
	
	for (int i = 0; i < rkeys.size(); i++) {
		rkey_t k = rkeys[i];
		fprintf(f, "%s, %d, %d, %d, %d, %d, %d, %08X, %d, %d\n", k.keyname, k.sp1, k.sp2, k.tH, k.t1, k.t0, k.bits, k.data, k.ep1, k.ep2);
	}
	
	fclose(f);
	
	return 0;
}
