
INCLUDE_PATH=-Iincludes -Iutils/include \
	-Id:/ztestfiles/libs/include \
	-Id:/dev_tools/openssl-3/x64/include

ProjectDir =../
SCRDIR_PREFIX =
SRCDIRS =./utils/socket

#SOURCES =utils/socket/myclient.c utils/socket/gui_client.c

DLLS= -liphlpapi -lws2_32
DLLS+=-Ld:/ztestfiles/libs
#DLLS += -lwpcap
DLLS += -llibssl-3-x64 -lcrypto-3-x64

exe: Main_net.o npcap_utils.o nets.o pro_frame.o
	gcc $^ -o ../out/a.exe $(DLLS)

client.exe: myclient.o gui_client.o
	gcc $^ -o ../out/client.exe -lws2_32 -mwindows

sslserver.exe: myserver.o
	g++ $^ -o ../out/$@  -Ld:/ztestfiles/libs -lws2_32 -lssl-3-x64 -lcrypto-3-x64

include ../template.mk