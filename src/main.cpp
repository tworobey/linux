#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);

        while (--n) {
            workers.emplace_back(fn);
        }

        fn();
    }

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем игру из JSON
        model::Game game = json_loader::LoadGame(argv[1]);

        // 2. Создаём io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Обработка сигналов (SIGINT, SIGTERM)
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code&, int) {
            ioc.stop();
            });

        // 4. Handler
        http_handler::RequestHandler handler{ game };

        // 5. Запуск HTTP сервера
        auto address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;
           
        std::cout << "Hello! Server is starting at port " << port << std::endl;
        std::cerr << "BEFORE ServeHttp" << std::endl;

        http_server::ServeHttp(ioc, { address, port },
            [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req),
                    std::forward<decltype(send)>(send));
            });

        std::cerr << "AFTER ServeHttp" << std::endl;

        // ВАЖНО: сигнал тестам
        std::cout << "Server has started..."sv << std::endl;

        // 6. Запуск потоков
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
            });

    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
