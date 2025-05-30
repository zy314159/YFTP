#include "../include/AsioIOServicePool.hpp"

#include "../include/logger.hpp"

using namespace std;
namespace Yftp {
AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : ioServices_(size), works_(size), nextIOService_(0) {
    LOG_INFO("AsioIOServicePool construct");
    LOG_INFO("AsioIOServicePool size is {}", size);
    for (std::size_t i = 0; i < size; ++i) {
        works_[i] = std::unique_ptr<Work>(new Work(ioServices_[i]));
    }

    // 遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < ioServices_.size(); ++i) {
        threads_.emplace_back([this, i]() { ioServices_[i].run(); });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    LOG_INFO("AsioIOServicePool destruct");
}

boost::asio::io_context& AsioIOServicePool::getIOService() {
    auto& service = ioServices_[nextIOService_++];
    if (nextIOService_ == ioServices_.size()) {
        nextIOService_ = 0;
    }
    return service;
}

void AsioIOServicePool::stop() {
    for (auto& work : works_) {
        work->get_io_context().stop();
        work.reset();
    }

    for (auto& t : threads_) {
        t.join();
    }
}
}  // namespace Yftp
