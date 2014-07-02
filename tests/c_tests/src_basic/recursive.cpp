//this calculates the recursively the fibanacci function

#include <stdio.h>

int fib(int n){

	if(n == 0) return 0;
	if(n == 1) return 1;
	if(n == 2) return 2;
	
	return fib(n-1) + fib(n-2);


}


int main(){

	int a = 5;
	a = fib(5);
	return 0;
}


