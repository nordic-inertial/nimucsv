# socketcan initialization in linux:
socketcan_init:
	sudo ip link set can0 up type can bitrate 1000000

TOOLS_INC = -Ican_if -Inimulib -I.
NIMUCSV_SRC = nimucsv.c nimulib/nimulib.c
KVASER_SRC = can_if/wrapcan_win_kvaser.c
PCAN_SRC = can_if/wrapcan_win_pcan.c
SOCKETCAN_SRC = can_if/wrapcan_linux_socketcan.c
CAN_HDR = can_if/wrapcan.h

nimucsv_linux: $(NIMUCSV_SRC) $(SOCKETCAN_SRC) $(CAN_HDR)
	gcc -Wall -DSOCKETCAN $(NIMUCSV_SRC) $(SOCKETCAN_SRC) $(TOOLS_INC) -o nimucsv -lm

nimucsv_win64_pcan: $(NIMUCSV_SRC) $(PCAN_SRC) $(CAN_HDR)
	x86_64-w64-mingw32-gcc -mno-ms-bitfields -Wall -DPCAN $(NIMUCSV_SRC) $(PCAN_SRC) $(TOOLS_INC) -o nimucsv-pcan.exe

nimucsv_win64_kvaser: $(NIMUCSV_SRC) $(KVASER_SRC) $(CAN_HDR)
	x86_64-w64-mingw32-gcc -mno-ms-bitfields -Wall -DKVASER $(NIMUCSV_SRC) $(KVASER_SRC) $(TOOLS_INC) -o nimucsv-kvaser.exe -Ltools/can_if/ext_lib -lcanlib32
