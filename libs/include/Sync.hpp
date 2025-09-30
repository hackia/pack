#pragma once

#include <string>
#include <cstdint>

namespace K {
    class Sync {
    public:
        /**
         * Starts a server that listens to on the specified port
         * @param port Port number to listen on
         * @return Status code (0 for success, 2 for error)
         */
        static uint16_t serve_once(uint16_t port);

        /**
         * Sends a message to a server
         * @param host Server host address
         * @param port Server port number
         * @param message Message to send
         * @return Status code (0 for success, 2 for error)
         */
        static int client_send(const std::string &host, uint16_t port, const std::string &message);
    };
}
