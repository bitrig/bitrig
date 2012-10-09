#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>

volatile static void init0(void) __attribute__ ((constructor));
volatile static void init1(void) __attribute__ ((constructor));
volatile static void init2(void) __attribute__ ((constructor));
volatile static void fini0(void) __attribute__ ((destructor));
volatile static void fini1(void) __attribute__ ((destructor));
volatile static void fini2(void) __attribute__ ((destructor));

int order = 0;

int a(void);

volatile void
init2(void) {
	if (order != 2)
		printf("%s: not called third\n", __func__);
	printf("%s\n", __func__);
	order++;
}
volatile void
init1(void) {
	if (order != 1)
		printf("%s: not called second\n", __func__);
	printf("%s\n", __func__);
	order++;
}
volatile void
init0(void) {
	if (order != 0)
		printf("%s: not called first\n", __func__);
	printf("%s\n", __func__);
	order++;
}

volatile void
fini0(void) {
	if (order != 3)
		printf("%s: not called first\n", __func__);
	printf("%s\n", __func__);
	order++;
}
volatile void
fini1(void) {
	if (order != 4)
		printf("%s: not called second\n", __func__);
	printf("%s\n", __func__);
	order++;
}
volatile void
fini2(void) {
	if (order != 5)
		printf("%s: not called third\n", __func__);
	order++;
	printf("%s\n", __func__);
	printf("all done order = %d\n", order);
}

int
main(int argc, char **argv)
{
	printf("running\n");

	a();

	return 0;
}
