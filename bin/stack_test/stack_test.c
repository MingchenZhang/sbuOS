void sys_print(char* str){
	__asm__ volatile (
		"movq $253, %%rdi;"
		"movq %0, %%rsi;"
		"int $0x80;"
		::"r"((unsigned long)str):"rsi", "rdi");
}

void sys_print_num(unsigned long num){
	__asm__ volatile (
		"movq $252, %%rdi;"
		"movq %0, %%rsi;"
		"int $0x80;"
		::"r"(num):"rsi", "rdi");
}

void stack(int count){
	char volatile space[3000];
	space[0] = count;
	if(count < 0){
		sys_print("done\n");
	}else{
		unsigned long rsp;
		__asm__ volatile ("movq %%rsp, %0": "=r"(rsp));
		sys_print("rsp reaches ");
		sys_print_num(rsp);
		sys_print("\n");
		stack(space[0]-1);
	}
}

int main(int argc, char**argv){
	stack(40);
	while(1);
}
