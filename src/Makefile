
INCLUDE_PATH=-Iincludes -Iutils/include -Id:/ztestfiles/libs/include
ProjectDir =../
SCRDIR_PREFIX =
SRCDIRS =utils/net

#SOURCES =utils/socket/myclient.c utils/socket/gui_client.c

DLLS=-Ld:/ztestfiles/libs -lwpcap -lws2_32 -liphlpapi
     

exe: Main_net.o npcap_utils.o nets.o
	gcc $^ -o ../out/a.exe $(DLLS)

include ../template.mk