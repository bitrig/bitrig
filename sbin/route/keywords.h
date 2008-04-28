/* $OpenBSD: keywords.h,v 1.21 2008/04/28 11:36:14 norby Exp $ */

/* WARNING!  This file was generated by keywords.sh  */

struct keytab {
	char	*kt_cp;
	int	kt_i;
};

enum {
	K_NULL,
	K_ADD,
	K_BLACKHOLE,
	K_CHANGE,
	K_CLONING,
	K_DELETE,
	K_DST,
	K_ENCAP,
	K_EXPIRE,
	K_FLUSH,
	K_GATEWAY,
	K_GENMASK,
	K_GET,
	K_HOST,
	K_HOPCOUNT,
	K_IFACE,
	K_INTERFACE,
	K_IFA,
	K_IFP,
	K_IN,
	K_INET,
	K_INET6,
	K_JUMBO,
	K_LABEL,
	K_LINK,
	K_LLINFO,
	K_LOCK,
	K_LOCKREST,
	K_MONITOR,
	K_MPATH,
	K_MPLS,
	K_MTU,
	K_NET,
	K_NETMASK,
	K_NOJUMBO,
	K_NOSTATIC,
	K_OUT,
	K_PREFIXLEN,
	K_PROTO1,
	K_PROTO2,
	K_RECVPIPE,
	K_REJECT,
	K_RTT,
	K_RTTVAR,
	K_SA,
	K_SENDPIPE,
	K_SHOW,
	K_SSTHRESH,
	K_STATIC,
	K_XRESOLVE,
};

struct keytab keywords[] = {
	{ "add",	K_ADD },
	{ "blackhole",	K_BLACKHOLE },
	{ "change",	K_CHANGE },
	{ "cloning",	K_CLONING },
	{ "delete",	K_DELETE },
	{ "dst",	K_DST },
	{ "encap",	K_ENCAP },
	{ "expire",	K_EXPIRE },
	{ "flush",	K_FLUSH },
	{ "gateway",	K_GATEWAY },
	{ "genmask",	K_GENMASK },
	{ "get",	K_GET },
	{ "host",	K_HOST },
	{ "hopcount",	K_HOPCOUNT },
	{ "iface",	K_IFACE },
	{ "interface",	K_INTERFACE },
	{ "ifa",	K_IFA },
	{ "ifp",	K_IFP },
	{ "in",	K_IN },
	{ "inet",	K_INET },
	{ "inet6",	K_INET6 },
	{ "jumbo",	K_JUMBO },
	{ "label",	K_LABEL },
	{ "link",	K_LINK },
	{ "llinfo",	K_LLINFO },
	{ "lock",	K_LOCK },
	{ "lockrest",	K_LOCKREST },
	{ "monitor",	K_MONITOR },
	{ "mpath",	K_MPATH },
	{ "mpls",	K_MPLS },
	{ "mtu",	K_MTU },
	{ "net",	K_NET },
	{ "netmask",	K_NETMASK },
	{ "nojumbo",	K_NOJUMBO },
	{ "nostatic",	K_NOSTATIC },
	{ "out",	K_OUT },
	{ "prefixlen",	K_PREFIXLEN },
	{ "proto1",	K_PROTO1 },
	{ "proto2",	K_PROTO2 },
	{ "recvpipe",	K_RECVPIPE },
	{ "reject",	K_REJECT },
	{ "rtt",	K_RTT },
	{ "rttvar",	K_RTTVAR },
	{ "sa",	K_SA },
	{ "sendpipe",	K_SENDPIPE },
	{ "show",	K_SHOW },
	{ "ssthresh",	K_SSTHRESH },
	{ "static",	K_STATIC },
	{ "xresolve",	K_XRESOLVE },
	{ 0, 0 }
};

