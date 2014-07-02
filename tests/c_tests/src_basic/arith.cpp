 #include <stdio.h>
 
 
 int main(){
 
	//basic arithmetic expressions
	int a0;
	int a1;
	
	
	//simple statements
	int plus = a0 + a1;
	int minus = a0 - a1;
	int mul = a0 * a1;
	int div = a0 / a1;
	int mod = a0 % a1;
	int and = a0 & a1;
	int or = a0 | a1;
	int xor = a0 ^ a1;
	int not = ~a0;
	
	//compound statements
	int comp_1 = (plus + minus) / (div * mod);
	int comp_2 = (xor - or) / (and / not);
 
 
	return 0;
 }