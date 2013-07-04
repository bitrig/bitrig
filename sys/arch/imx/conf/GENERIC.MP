include		"arch/imx/conf/generic.imx"
config		bsd	root on sd0a swap on sd0b

option	MULTIPROCESSOR
cpu*		at mainbus?
