cmd_Release/obj.target/hfpio.node := g++ -shared -pthread -rdynamic  -Wl,-soname=hfpio.node -o Release/obj.target/hfpio.node -Wl,--start-group Release/obj.target/hfpio/src/hfpio.o Release/obj.target/hfpio/src/utils.o -Wl,--end-group -lsbc -lasound