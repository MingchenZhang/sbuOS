int main(int argc, char**argv){
	for(long long i=0; ;i++){
		if(i%0x14000000 == 0){
			// kprintf("test_func_2 reach %x\n", i);
			__asm__ volatile (
			"movq $102, %rdi;"
			"int $0x80;");
		}
	}
}