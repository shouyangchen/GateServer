#ifndef GRPCMRG_HPP
#define GRPCMRG_HPP
#include <grpc++/grpc++.h>
#include <thread>
#include <grpc++/channel.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "singleton.h"
#include <queue>
#include <mutex>
#include <atomic>
#include "ConfiMgr.h"
#include <condition_variable>
class gRPCServerPool : public singleton<gRPCServerPool> // gRPC服务�?
{
private:
    friend class singleton<gRPCServerPool>;
    gRPCServerPool(int size = 300);
    std::queue<std::unique_ptr<message::StatusService::Stub>> stub_pool;
    std::queue<std::shared_ptr<grpc::Channel>> channel_pool;
    std::condition_variable cv_channel_pop;
    std::condition_variable cv_stub_pop;
    std::mutex mt_channel;
    std::mutex mt_stub;
    std::atomic<bool> is_stop;

public:
    std::unique_ptr<message::StatusService::Stub> get_stub();
    std::shared_ptr<grpc::Channel> get_Channel();
    void release_stub(std::unique_ptr<message::StatusService::Stub> stub);
    void release_Channel(std::shared_ptr<grpc::Channel> channel);
    void stop();
    gRPCServerPool(gRPCServerPool &&other) = delete;
    gRPCServerPool(gRPCServerPool &other) = delete;
    gRPCServerPool& operator=(const gRPCServerPool &) = delete;
    ~gRPCServerPool() = default;
};

#endif