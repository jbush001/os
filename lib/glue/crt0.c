extern void main();
extern void __syslib_init();

typedef void (*ctor_dtor_hook)();
extern ctor_dtor_hook __CTORS__[];
extern ctor_dtor_hook __DTORS__[];

void _start()
{
	ctor_dtor_hook *hook;

	__syslib_init();

	for (hook = __CTORS__; *hook; hook++)
		 (*hook)();

	main();

	for (hook = __DTORS__; *hook; hook++)
		 (*hook)();
}
