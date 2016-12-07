cmd_Release/obj.target/alsa.node := g++ -shared -pthread -rdynamic  -Wl,-soname=alsa.node -o Release/obj.target/alsa.node -Wl,--start-group Release/obj.target/alsa/src/alsa.o -Wl,--end-group -lasound
