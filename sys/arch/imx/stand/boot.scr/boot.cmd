setenv loadaddr 0x10800000 ;
fatload mmc 1:1 ${loadaddr} barebox.bin ;
go ${loadaddr}
