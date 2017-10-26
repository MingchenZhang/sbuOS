int main(int argc, char**argv){
	for(long long i=0; ;i++){
		if(i%0x17000000 == 0){
			__asm__ volatile (
			"movq $100, %rdi;"
			"int $0x80;");
		}
	}
}