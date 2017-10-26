int main(int argc, char**argv){
	for(; ;){
		__asm__ volatile (
			"movq $254, %rdi;"
			"movq $1, %rsi;"
			"int $0x80;");
		__asm__ volatile (
			"movq $102, %rdi;"
			"int $0x80;");
	}
}