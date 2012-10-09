#include <stdio.h>
volatile static void libainit0(void) __attribute__ ((constructor));
volatile static void libainit1(void) __attribute__ ((constructor));
volatile static void libainit2(void) __attribute__ ((constructor));
volatile static void libafini0(void) __attribute__ ((destructor));
volatile static void libafini1(void) __attribute__ ((destructor));
volatile static void libafini2(void) __attribute__ ((destructor));

int libaorder = 0;

int a(void);
int
a(void)
{
	printf("liba: %d\n", libaorder);
	return 0;
}

volatile void
libainit2(void) {
	if (libaorder != 2)
		printf("%s: not called third\n", __func__);
	printf("%s\n", __func__);
	libaorder++;
}
volatile void
libainit1(void) {
	if (libaorder != 1)
		printf("%s: not called second\n", __func__);
	printf("%s\n", __func__);
	libaorder++;
}
volatile void
libainit0(void) {
	if (libaorder != 0)
		printf("%s: not called first\n", __func__);
	printf("%s\n", __func__);
	libaorder++;
}

volatile void
libafini0(void) {
	if (libaorder != 3)
		printf("%s: not called first\n", __func__);
	printf("%s\n", __func__);
	libaorder++;
}
volatile void
libafini1(void) {
	if (libaorder != 4)
		printf("%s: not called second\n", __func__);
	printf("%s\n", __func__);
	libaorder++;
}
volatile void
libafini2(void) {
	if (libaorder != 5)
		printf("%s: not called third\n", __func__);
	libaorder++;
	printf("%s\n", __func__);
	printf("all done libaorder = %d\n", libaorder);
}
