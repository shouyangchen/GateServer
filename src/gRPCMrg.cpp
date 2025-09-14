#include "gRPCMrg.h"
#include <utility>

gRPCServerPool::gRPCServerPool(int size) : singleton<gRPCServerPool>(), is_stop(false)
{
    try
    {
        auto host = ConfiMgr::getInstance()["StautsServer"]["host"];
        auto port = ConfiMgr::getInstance()["StautsServer"]["port"];
        for (int i = 0; i < size; i++)
        {
            auto address = host + ":" + port;
            auto Channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            this->channel_pool.push(Channel);
            this->stub_pool.push(message::StatusService::NewStub(Channel));
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what();
    }
}

std::unique_ptr<message::StatusService::Stub> gRPCServerPool::get_stub()
{
    std::unique_lock<std::mutex> lock(this->mt_stub);
    if (!this->is_stop.load(std::memory_order_acquire))
    {
        this->cv_stub_pop.wait(lock, [&]()
                               { return (!this->stub_pool.empty()) || this->is_stop.load(std::memory_order_acquire); });
        if (!this->is_stop.load(std::memory_order_acquire))
        {
            auto stub_ptr = std::move(this->stub_pool.front());
            this->stub_pool.pop();
            lock.unlock();
            return stub_ptr;
        }
        return nullptr;
    }
    else
    {
        std::cerr << "server is stop!" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<grpc::Channel> gRPCServerPool::get_Channel()
{
    std::unique_lock<std::mutex> lock(this->mt_channel);
    if (!this->is_stop.load(std::memory_order_acquire))
    {
        this->cv_channel_pop.wait(lock, [&]()
                                  { return (!this->channel_pool.empty()) || this->is_stop.load(std::memory_order_acquire); });
        if (!this->is_stop.load(std::memory_order_acquire))
        {
            auto channel_ptr = std::move(this->channel_pool.front());
            this->channel_pool.pop();
            lock.unlock();
            return channel_ptr;
        }
        return nullptr;
    }
    else
    {
        std::cerr << "server is stop!" << std::endl;
        return nullptr;
    }
}

void gRPCServerPool::release_stub(std::unique_ptr<message::StatusService::Stub> stub)
{
    std::unique_lock<std::mutex> lock(this->mt_stub);
    if (!this->is_stop.load(std::memory_order_acquire))
    {
        this->stub_pool.push(std::move(stub));
        this->cv_stub_pop.notify_one();
    }
    else
    {
        std::cerr << "server is stop!" << std::endl;
    }
}

void gRPCServerPool::release_Channel(std::shared_ptr<grpc::Channel> channel)
{
    std::unique_lock<std::mutex> lock(this->mt_channel);
    if (!this->is_stop.load(std::memory_order_acquire))
    {
        this->channel_pool.push(std::move(channel));
        this->cv_channel_pop.notify_one();
    }
    else
    {
        std::cerr << "server is stop!" << std::endl;
    }
}

void gRPCServerPool::stop()
{
    this->is_stop.store(true, std::memory_order_release);
    this->cv_channel_pop.notify_all();
    this->cv_stub_pop.notify_all();
}