//producer consumer blur application

 #include <stdio.h>
 
 #define X_WIDTH 800
 #define Y_WIDTH 600
 
 int main(){
 
	//arrays - static arrays
	int input[X_WIDTH][Y_WIDTH];
	int blur_x[X_WIDTH][Y_WIDTH];
	int blur_y[X_WIDTH][Y_WIDTH];
	
	//we do not initialize the values - if initialized there is a problem 
	
	//1*3 point stencil computation
	for(int i=1; i<X_WIDTH - 1; i++){
		for(int j=1; j<Y_WIDTH - 1; j++){
			blur_x[i][j] = (input[i-1][j] + input[i][j] + input[i+1][j])/3;
		}
	}
	
	//3*1 point stencil computation
	for(int i=1; i<X_WIDTH - 1; i++){
		for(int j=1; j<Y_WIDTH - 1; j++){
			blur_y[i][j] = (input[i][j-1] + input[i][j] + input[i][j+1])/3;
		}
	}
	
	
	return 0;
	
	
 }