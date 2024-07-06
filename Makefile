CPPFLAGS=-g -I/usr/include/jsoncpp
LDLIBS=-ljsoncpp

ttov: ttov.cpp packet.cpp debug.cpp server_socket.cpp services.cpp
