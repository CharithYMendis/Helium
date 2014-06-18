//decompilation of the function - e450
//assume that all xmms are initialized to memory pointers

m128 * xmm0; 
m128 * xmm1; 
m128 * xmm2; 
m128 * xmm3;
m128 * xmm4;
m128 * xmm5;
m128 * xmm6;
m128 * xmm7;

void func(uint32 eax, uint32 edx, uint ecx, 
			uint32 P4, //parameter 4
			uint32 P5, //parameter 5
			uint32 P6, //parameter 6
			uint32 P7  //parameter 7){
			
		uint32 stack_array[(0x40f8 + 0x18)/4];

		if(2*P7 + P4 <= 0x2000){
		
			initialize_mem((void *)stack_array + 0x20, 2*(2*P7 + P4), 0);
		
			//pointers
			void * stack_offset = (void *)stack_array + 0x20;
		
			//loop 1 - induction variables
			int loop_1 = 0;
			int loop_2 = 0;
			void * ptr = eax - P7 - P5*P7;
			*xmm0 = 0;
			*xmm1 = 0;
			
			while ( loop_1 < 2*P7 + 1) {
	
				int var_1 = 2*P7 + P4;  
				
				if( var_1  != 0){  
					
					if( var_1 < 8 ){
						goto EF65;
					}
					else{
						void* ptr += loop_2 ; 
						uint comp_1 = (2*P7 + P4) - (2*P7 + P4) ^ 0x7;
						uint comp_2 = (2*P7 + P4);
						uint i = 0;
						m128 * xmm1;
						
						while(comp_1 > i){
							*xmm1 = *((m128 *)(ptr + i));
							jumble_bytes(xmm1,xmm0,8);
							*((m128 *)(stack_offset + i * 2)) += *xmm1;
							i += 8;
						}
						
						while(comp_1 < comp_2){
							//take a byte at a time and put a word (16 bytes) in memory
							*((word *)(stack_offset + comp_1 * 2)) += *((word *)(ptr + comp_1)) & 0x00ff;
							comp_1++;
						}
						loop_2 += P5;
						loop_1++;
					}
				}
				else{
					break;
				}
			}
			

			for(int i=0;i<7;i++){
				*((word *)xmm4 + i) = (2^23/((2*P7+1)^2)) & 0xffff;
				*((word *)xmm3 + i) = (2^7/((2*P7+1)^2)) & 0xffff;
			}

			//some signed/unsigned loop
			*xmm0 = (2*P7 + 1)^2/2;
			*xmm1 = *xmm0;
			*xmm2 = 0;
			
			uint32 outer_i = 0;
			uint32 eax_inc = 0;
			uint32 edx_inc = 0;
			//outer loop
			while(outer_i < ecx - 1){
			
				//inner loop 1
				uint32 loop_i = (2*P7 + 1) - (2*P7 + 1) ^ 0x3
				uint32 i = 0;
				while(i < loop_i){
					*xmm5 = *((uin64 *)(stack_offset + i * 2));
					i += 4;
					jumble_words(xmm5,xmm2,4);
					*xmm1 += *xmm5;
				}

				*xmm1 = (*xmm1 + *xmm1 >> 64) + (*xmm1 + *xmm1 >> 64) >> 32;
				uint32 reduction = xmm1 & 0xffffffff;
				
				while(loop_i < (2*P7 + 1)){
					reduction += *((word *)(stack_offset + loop_i * 2));
					loop_i++;
				}

				//main inner loop 2
				//inner loop limit
				uint32 loop_limit = (P4 - (P4 ^ 0xf) + 0xf) >> 4
				uint32 i = 0;
				uint32 depend = reduction;
				
				word * mem1 = (word *)(stack_offset + P7*4);
				word * mem2 = (word *)(stack_offset);
				m128 * mem3 = (m128 *)(edx);
				m128 * mem4 = (m128 *)(eax + P5 - P7 + P5 * P7);
				m128 * mem5 = (m128 *)(eax - P5 * P7 - P7);
				
				word * xmm0_w = (word *)xmm0;
				word * xmm1_w = (word *)xmm1;
				word * xmm6_w = (word *)xmm6;
				word * xmm5_w = (word *)xmm5;
				word * xmm4_w = (word *)xmm4;
				
				while(j < loop_limit){
					
					uint32 index = j*32;
					*xmm6 = *xmm3;
					*xmm5 = *xmm3;
					

					for(int i=7; i>=0; i--){
						xmm0_w[i] = depend ;
						depend = *((word *)((void *)mem1 + index + (8 - i)*2)) + xmm0_w[i] - *((word *)((void *)mem2 + index + (8 - i)*2));
					}

					xmm1_w[0] = mem1[8] + xmm0[7] - mem2[8];

					for(int i=7; i>=0; i--){
						xmm0_w[i] = depend ;
						depend = *((word *)((void *)mem1 + index + (8 - i)*2 + 16)) + xmm1_w[i] - *((word *)((void *)mem2 + index + (8 - i)*2 + 16)); 
					}

					for(int i=0; i<8; i++){
						xmm6_w[i] = (xmm6_w[i]*xmm0_w[i])&0x0000ffff;
						xmm5_w[i] = (xmm5_w[i]*xmm1_w[i])&0x0000ffff;
						xmm0_w[i] = ((xmm0_w[i]*xmm4_w[i])&0xffff0000)>>16;
						xmm1_w[i] = ((xmm1_w[i]*xmm4_w[i])&0xffff0000)>>16;
					}

					for(int i=0; i<8 ;i++){
					    //make the word results 8 bit (remember there is the sign bit)
						xmm6_w[i] = (xmm6_w[i] + xmm0_w[i]) >> 7;
						xmm5_w[i] = (xmm5_w[i] + xmm1_w[i]) >> 7;
					}
					
					//now make the signed word operands bytes (already they are signed byte due to the left shift)
					//packusbw
					packusbw(xmm6,xmm5);
					
					//writing back to memory
					*((m128 *)((void *)mem3 + eax_inc + j*16)) = *xmm6;
					
					//reading from the heap
					*xmm7 = *(uint64 *)((void *)mem4 + edx_inc);
					*xmm5 = *(uint64 *)((void *)mem5 + edx_inc);
					
					*xmm6 = *xmm7;
					*xmm0 = *xmm5;
					
					jumble_bytes(xmm6,xmm1,4);
					jumble_bytes(xmm7,xmm1,8);
					jumble_bytes(xmm0,xmm1,4);
					jumble_bytes(xmm5,xmm1,8);
					
					for(int i=0;i<8;i++){
						xmm6_w[i] += *(word *)((void *)stack_offset + index + i*2) - xmm0_w[i];
						xmm7_w[i] += *(word *)((void *)stack_offset + index + i*2 + 0x10) - xmm5_w[i];
					}
					
					*((m128 *)((void *)stack_offset + index + i*2)) = xmm6;
					*((m128 *)((void *)stack_offset + index + i*2 + 0x10)) = xmm6;
					
					j++;
				}
				
				//do it for the remaining part of the loop as well. -> need to simplify the structure
				//yet to code
				
				//another small loop coming here
				void * addr_1 = stack_offset + 2*(2*P7 + P4);
				void * addr_2 = eax + P5 - P7 + P7*P5;
				void * addr_3 = eax - P5*P7 - P7 + P4;
				uint32 counter = 0;
				
				while(counter < P7*2){
					*((word *)addr_1) += *((byte *)addr_2) - *((byte *)addr_3);
					counter++;
					addr_2++;
					addr_3++;
					addr_1 += 2;
				}

				outer_i++;
				eax_inc += P6;
				edx_inc += P5;
				
			}
			
			//another small loop coming here - I think boundary conditions
			uint32 counter_loop = 0;
			while(counter_loop < P4){
				//do some processing
			}
			
		}			
}

//packusbw
void packusbw(m128 * mem1, m128 * mem2){
	word mem1_w[8];
	word mem2_w[8];
	
	for(int i=0;i<8;i++){
		mem1_w[i] = ((word *)mem1 + i);
		mem2_w[i] = ((word *)mem2 + i);
	}
	
	for(int i=0;i<4;i++){
		byte first = mem1_w[2*i] > 0xff ? 0xff : (mem1_w[2*i] < 0 ? 0 : mem1_w[2*i] & 0xff);
		byte second = mem1_w[2*i + 1] > 0xff ? 0xff : (mem1_w[2*i + 1] < 0 ? 0 : mem1_w[2*i + 1] & 0xff);
		*((word *)mem1) = first |  second<<8;
	}
	
	for(int i=0;i<4;i++){
		byte first = mem2_w[2*i] > 0xff ? 0xff : (mem2_w[2*i] < 0 ? 0 : mem2_w[2*i] & 0xff);
		byte second = mem2_w[2*i + 1] > 0xff ? 0xff : (mem2_w[2*i + 1] < 0 ? 0 : mem2_w[2*i + 1] & 0xff);
		*((word *)mem1) = first |  second<<8;
	}

}

//trivial non-vectorized version
void initializa_mem_nonvec(void * start_addr, uint length, byte fill){
	for(int i=0;i<length;i++){
		*(byte *)(start_addr + i) = fill;
	}
}

//punpcklbw
void jumble_bytes(m128 * mem1, m128 * mem2, uint32 start){
	byte mem1_8[16];
	byte mem2_8[16];
	
	for(int i=0;i<16;i++){
		mem1_8[i] = *((byte *)(mem1) + i);
		mem2_8[i] = *((byte *)(mem2) + i);
	}
	
	for(int i=0;i<8;i++){
		*((byte *)mem1 + 2*i) = mem1_8[start+i];
		*((byte *)mem1 + 2*i + 1) = mem2_8[start+i];
	}
}

//punpcklwd
void jumble_words(m128 * mem1, m128 * mem2, uint32 start){
	word mem1_8[8];
	word mem2_8[8];
	
	for(int i=0;i<8;i++){
		mem1_8[i] = *((word *)(mem1) + i);
		mem2_8[i] = *((word *)(mem2) + i);
	}
	
	for(int i=0;i<4;i++){
		*((word *)mem1 + 2*i) = mem1_8[start+i];
		*((word *)mem1 + 2*i + 1) = mem2_8[start+i];
	}
} 



//final high level algorithm
1. allocate space in the STACK (40F8h)
2. if (2*P7 + P4) <= 2000h proceed // reason we are going to allocate 40F8h in the stack for storing the results and need 4000h for storing intermediate results
3. initialize_mem( esp +20 , 2*(2*P7 + P4) , 0 )  //initialize memory withs zeros for 2*(2*P7 + P4)
4. LOOP1 as above - we are just gettting valuews from heap and and adding them one by one in P5 location increments. (stride is P5) also note that byte values are added and 
that words are allocated for the addition to account for overflows. The loop iterates for 2*P7 + 1 iterations.