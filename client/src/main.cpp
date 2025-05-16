#include "../include/cli.hpp"
#include <boost/program_options.hpp>

namespace fs = std::filesystem;
using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;

    std::string host = "127.0.0.1";
    short port = 8080;

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Show help message")(
        "host,H", po::value<std::string>(&host)->default_value("127.0.0.1"),
        "Server host")("port,p", po::value<short>(&port)->default_value(8080),
                       "Server port");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    try {
        CommandLineInterface cli(host, port);
        cli.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
