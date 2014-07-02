//multiple function calls
#include <stdio.h>

//this will calculate the nth traingle number; note that the value of a is manipulated in place in the calc function

bool calc(int &limit, int &a){
	if(limit <= 0) return false;
	
	a+=limit--;
}


int main(){

	int a;
	int limit = 10;
	while(calc(limit,a)){
		if(limit>0){
			a+=limit--;
		}
	}


}