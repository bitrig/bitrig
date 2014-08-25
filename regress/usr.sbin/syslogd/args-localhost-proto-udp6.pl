# The client writes a message to Sys::Syslog native method.
# The syslogd writes it into a file and through a pipe.
# The syslogd passes it via explicit IPv6 UDP to localhost.
# The server receives the message on its UDP socket.
# Find the message in client, file, pipe, syslogd, server log.
# Check that localhost gets resolved to the ::1 address.

use strict;
use warnings;

our %args = (
    syslogd => {
	loghost => '@udp6://localhost.:$connectport',
    },
    server => {
	listen => { domain => AF_INET6, addr => "::1" },
    },
);

1;
