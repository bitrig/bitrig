struct tls_be_scared {
	int i;
	double d;
	int (*func)(int);
};

extern __thread struct tls_be_scared __tls_be_scared;
