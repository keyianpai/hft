#include <zmq.hpp>

int main() {
    zmq::context_t context(1);
    zmq::socket_t sub(context, ZMQ_XSUB);
    sub.bind("ipc://data_sub");
    zmq::socket_t pub(context, ZMQ_XPUB);
    pub.bind("ipc://data_pub");
    zmq::proxy(sub, pub, NULL);
    return 0;
}
