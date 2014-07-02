 #include <stdio.h>
 
 #define X_WIDTH 10
 #define Y_WIDTH 10

//arrays - static arrays
int input[X_WIDTH][Y_WIDTH];
int output[X_WIDTH][Y_WIDTH];
 
 int main(){
 

	//9 point stencil computation
	for(int i=1; i<X_WIDTH - 1; i++){
		for(int j=1; j<Y_WIDTH - 1; j++){
			output[i][j] = (input[i-1][j-1] + input[i][j-1] + input[i+1][j-1]
						+ input[i-1][j] + input[i][j] + input[i+1][j]
						+ input[i-1][j+1] + input[i][j+1] + input[i+1][j+1])/9;
		}
	}

	printf("hello\n");
	
	return 0;
	
	
 }