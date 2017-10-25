#include <unistd.h>
#include <stdlib.h>

void _start(void) {
	// call main() and exit() here
	int* argc_a;
	int argc;
	char** argv;
	char** env;
	__asm__("movq %%rsp, %0"
	  :"=rm"(argc_a)::);
	argc_a += 2;
	argc = *(argc_a);
	argv = ((char**)argc_a)+1;
	env = ((char**)argc_a)+argc+2;
	environ = env;
	exit(main(argc, argv, env));
}