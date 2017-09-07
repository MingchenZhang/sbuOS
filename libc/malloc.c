#include<stdio.h>
#include<unistd.h>
#include <sys/mman.h>

void* start_of_available_addr=0;
void* end_of_available_addr;

//PROT_READ  0X1 
//PROT_WRITE 0X2
//PROT_EXEC 0X4
//MAP_SHARED 0X01
//MAP_ANONYMOUS 0X20
//MAP_SHARED 0x1 MAP_PRIVATE 0x2 has to be specified 

/*void* malloc (size_t size){
	void* returnValue;
	if(start_of_available_addr==0 || start_of_available_addr+size>end_of_available_addr){
		void* acquired_addr=mmap(0,4096,0x1|0x2|0x04, 0x20|0x01,-1, 0); 
		start_of_available_addr=acquired_addr+size;
		end_of_available_addr=acquired_addr+4096;
		returnValue=acquired_addr;
	}
   else{
   		returnValue=start_of_available_addr;
	    start_of_available_addr=start_of_available_addr+size;   

   }
   return returnValue;
}

void free(void* pointer){
	//DO NOTHING
}
*/


// sbrk 

void* malloc (size_t size){
	void* returnValue;
	if(start_of_available_addr==0 || start_of_available_addr+size>end_of_available_addr){
		//void* sbrk(4096);
		void* current_break=sbrk(0);
		void* sbrk_return=sbrk(4096);
		if(sbrk_return==current_break){
		    void*acquired_addr=sbrk_return;
			start_of_available_addr=acquired_addr+size;
			end_of_available_addr=acquired_addr+4096;
			returnValue= acquired_addr;
		}
		
		else{
			puts("sbrk failed\n");
			returnValue=NULL;
		}	
	}
   else{
   		returnValue=start_of_available_addr;
	    start_of_available_addr=start_of_available_addr+size;   
	}
   return returnValue;
}

void free(void* pointer){
	//DO NOTHING
}




