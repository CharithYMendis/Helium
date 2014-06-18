e62f - e6bd loop 

loop_i = 0; // this time entering the loop - esi
loop_1 = 0; // ecx
ori_eax = <eax>; //eax original value that was live when it entered this subroutine
stack_offset = esp + 0x20;
xmm0 = 0;

//two's complement - 1's complement + 1
//main idea - interleave 0's in between data values and put them into stack
// ...<0-byte><data-byte><0-byte><data-byte>....
// I think data-byte first then 0-byte

//LOOP1
while ( loop_i < 2* P7 + 1) {
	
	int var_1 = P7 * 2 + P4;  //eax at start
	
	if( var_1  != 0){   //eax != 0
		
		if( var_1 < 8 ){
			goto EF65;
		}
		else{
			uint* ptr = loop_1 + ori_eax - P7 - P5*P7; //edx
			uint comp_1 = (P7*2 + P4) - (P7*2 + P4) ^ 0x7;
			uint comp_2 = (P7*2 + P4);
			uint i = 0;
			
			while(comp_1 > i){
				m128 xmm1 = *(ptr + i);
				jumblelowbytes(xmm1,xmm0);
				*(stack_offset + i * 2) += xmm1;
				i += 8;
			}
			
			while(comp_1 < comp_2){
				//take a byte at a time and put a word (16 bytes) in memory
				*(stack_offset + comp_1*2) += *(ptr + comp_1) & 0x00ff;
				comp_1++;
			}
			
			loop_1 += P5;
			loop_i++;
		}
	}
	else{
		break;
	}
}

//loop non-verctorized - see why 2*(2*P7 + P4) times in the stack was initialized? :)
if(P7*2 + P4 != 0){
	loop_inc = 0;
	while(i < 2*P7 + 1){
		if(P7*2 + P4 < 8) goto EF65;
		from_addr = ori_eax + loop_inc - P7 - P7*P5; 
		to_addr = esp + 20;
		for(int j = 0; j < P7*2 + P4 ;j++){
			byte val = *(from_addr + j);
			*(to_addr + 2*j) += val;  //overflowed to the next byte that is why they are filling zeros in the middle
		}
		loop_inc += P5;
		i++;
	}
}


//at the end of the loop ptr = ori_eax - P7 - P5*P7 + P5*(2*P7+1)
//ptr = ori_eax + P5 - P7 + P5*P7 

//after loop code
//populating xmm4

uint16 xmm4[8];
uint16 xmm3[8];

for(int i=0;i<7;i++){
	xmm4[i] = (2^23/((2*P7+1)^2)) & 0xffff;
	xmm3[i] = (2^7/((2*P7+1)^2)) & 0xffff;
}

//some signed/unsigned loop
xmm0 = (2*P7 + 1)^2/2;
xmm1 = xmm0;
xmm2 = 0;

uint loop_limit = (2*P7 + 1) - (2*P7 + 1) ^ 0x3
uint i = 0;
while(i < loop_limit){
	xmm5 = *((uin64 *)(esp + 20 + i * 2));
	i += 4;
	jumblelowwords(xmm5,xmm2);
	xmm1 += xmm5;
}

xmm1 = (xmm1 + xmm1 >> 64) + (xmm1 + xmm1 >> 64) >> 32;
eax = xmm1 & 0xffffffff;

//add the rest of the loop iterations up to (2*P7 + 1)

void jumblelowbytes (m128 * mem1,m128 * mem2){
}

//dynamic function - standard calling convention - esi,edi,ebx registers callee saved
//this will fill the memory region with 'fill' starting at 'start_addr' for the length 'length'
void initialize_mem(void* start_addr,uint length,byte fill){

	if(length > 0x10){  //filled in multiples of 16
		xmm0 = fill; //fill the entire xmm0 register with fill byte value
		if (start_addr & 0xf != 0){ //starting address not 16 byte aligned
			start_addr = start_addr - (start_addr & 0xf) + 0x10; //go to the next 16 byte aligned address
			length = length - (0x10 - (start_addr & 0xf)); // reduce length by the amount skipped
		}
		
		offset = length & 0xf;
		aligned_length = length - (length & 0xf); //make the length 16 byte aligned

		int i=0;	
		while(i < aligned_length){
			*((_m128*)start_addr) = xmm0;
			start_addr += 0x10;
			i += 0x10;
		}

		if(offset != 0){
			*(start_addr + length - 0x10) = (uint32)xmm0;
		}
	}
	else{
		//not covered
	}
}
