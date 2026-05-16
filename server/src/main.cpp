#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <optional>
#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"
#include "ticker.h"
#include "extra_data.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace json = boost::json;
namespace po = boost::program_options;

namespace {

std::string MyFormatter(const boost::log::record_view& rec,
                        boost::log::formatting_ostream& strm)
{
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time_point));

    json::object obj;
    obj["timestamp"] = buf;
    obj["message"] = boost::log::extract<std::string>("Message", rec).get();

    strm << json::serialize(obj);
    return {};
}

struct Args {
    std::optional<int> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn = false;
};

std::optional<Args> ParseArgs(int argc, const char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>(), "set tick period (milliseconds)")
        ("config-file,c", po::value<std::string>(), "set config file path")
        ("www-root,w", po::value<std::string>(), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    Args args;

    if (vm.count("tick-period"))
        args.tick_period = vm["tick-period"].as<int>();

    if (vm.count("config-file"))
        args.config_file = vm["config-file"].as<std::string>();
    else
        throw std::runtime_error("config-file is required");

    if (vm.count("www-root"))
        args.www_root = vm["www-root"].as<std::string>();
    else
        throw std::runtime_error("www-root is required");

    args.randomize_spawn = vm.count("randomize-spawn-points") > 0;

    return args;
}

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    for (auto& t : workers) {
        t.join();
    }
}

} // namespace

int main(int argc, const char* argv[]) {
    try {
        auto args = ParseArgs(argc, argv);
        if (!args) return EXIT_SUCCESS;

        boost::log::add_console_log(
            std::cout,
            boost::log::keywords::format = &MyFormatter,
            boost::log::keywords::auto_flush = true
        );

        BOOST_LOG_TRIVIAL(info) << "server started";

        auto result = json_loader::LoadGame(args->config_file);
        model::Game& game = result.game;
        extra_data::MapExtraData& extra_data = result.extra_data;
        game.SetRandomizeSpawn(args->randomize_spawn);

        fs::path static_root = args->www_root;
        bool tick_auto_mode = args->tick_period.has_value();

        const unsigned threads = std::thread::hardware_concurrency();
        net::io_context ioc(threads);
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](auto, auto) {
            ioc.stop();
        });

        auto api_strand = net::make_strand(ioc);

        http_handler::RequestHandler handler{game, extra_data, static_root, tick_auto_mode};

        auto logging_handler = [&](auto&& req, auto&& send) {
            auto start = std::chrono::steady_clock::now();

            auto logging_send = [&](auto&& response) {
                auto end = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                std::string content_type = "unknown";
                if (response.find(boost::beast::http::field::content_type) != response.end()) {
                    content_type = std::string(response[boost::beast::http::field::content_type]);
                }

                json::object data;
                data["response_time"] = ms;
                data["code"] = response.result_int();
                data["content_type"] = content_type;

                json::object log;
                log["message"] = "response sent";
                log["data"] = data;

                std::cout << json::serialize(log) << std::endl;

                send(std::forward<decltype(response)>(response));
            };

            return handler(
                std::forward<decltype(req)>(req),
                logging_send
            );
        };

        // Запускаем тикер если задан --tick-period
        if (args->tick_period) {
            auto ticker = std::make_shared<Ticker>(
                api_strand,
                std::chrono::milliseconds(*args->tick_period),
                [&game](std::chrono::milliseconds delta) {
                    game.Tick(static_cast<double>(delta.count()));
                }
            );
            ticker->Start();
        }

        auto address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;
        http_server::ServeHttp(ioc, {address, port}, logging_handler);
        RunWorkers(std::max(1u, threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

