#pragma once
#include <vector>
#include <filesystem> // Pour C++17 <filesystem>
#include <fstream>    // Pour std::ofstream
#include <cstddef>    // Pour std::byte

// --- AJOUTS POUR ASIO ET LES COROUTINES ---
#include <asio/io_context.hpp>
#include <asio/thread_pool.hpp>
#include <asio/dispatch.hpp>
#include <asio/co_spawn.hpp>      // Pour lancer les coroutines
#include <asio/awaitable.hpp>     // Pour le type de retour de la coroutine
#include <asio/use_awaitable.hpp> // Pour "co_await"
using namespace std;

namespace k {
    class Sender {
    public:
        /**
         *
         * @param files the file to send
         */
        explicit Sender(const vector<byte> &files);
        int send();
    private:
        vector<byte> to_send = {};
    };
}

using namespace k;
int a () {
    auto s = Sender({});
    return s.send();
}