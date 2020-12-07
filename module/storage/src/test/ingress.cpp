#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "message.hpp"
#include "reg.hpp"

int main(int argc, char *argv[]) {
    hdtn::hdtn_regsvr _reg;
    _reg.init("tcp://127.0.0.1:10140", "ingress", 10149, "push");
    _reg.reg();
    zmq::context_t ctx;
    zmq::socket_t socket(ctx, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:10149");

    int rfd = open("/dev/urandom", O_RDONLY);
    if (rfd < 0) {
        perror("Failed to open /dev/urandom: ");
        return -1;
    }

    char data[8192];
    read(rfd, data, 8192);

    while (true) {
        char ihdr[sizeof(hdtn::block_hdr)];
        hdtn::block_hdr *block = (hdtn::block_hdr *)ihdr;
        memset(ihdr, 0, sizeof(hdtn::block_hdr));
        block->base.type = HDTN_MSGTYPE_STORE;
        block->flow_id = rand() % 65536;
        socket.send(ihdr, sizeof(hdtn::block_hdr), ZMQ_MORE);
        socket.send(data, 1024, 0);
    }

    return 0;
}