cmd_Release/obj.target/a2dp.node := g++ -shared -pthread -rdynamic  -Wl,-soname=a2dp.node -o Release/obj.target/a2dp.node -Wl,--start-group Release/obj.target/a2dp/src/a2dp.o -Wl,--end-group -lsbc
