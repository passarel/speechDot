cmd_Release/obj.target/dbus.node := g++ -shared -pthread -rdynamic  -Wl,-soname=dbus.node -o Release/obj.target/dbus.node -Wl,--start-group Release/obj.target/dbus/src/dbus.o Release/obj.target/dbus/src/connection.o Release/obj.target/dbus/src/signal.o Release/obj.target/dbus/src/encoder.o Release/obj.target/dbus/src/decoder.o Release/obj.target/dbus/src/introspect.o Release/obj.target/dbus/src/object_handler.o -Wl,--end-group -ldbus-1 -lexpat
