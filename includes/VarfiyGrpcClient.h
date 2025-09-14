#ifndef VarfiyGrpcClient_H
#define VarfiyGrpcClient_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include"LogSystem.h"
#include "const.h"
#include "ConfiMgr.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPConPool
{
public:
	RPConPool(size_t poolsize, std::string host, std::string port) : poolSize_m(poolsize), host_m(host), port_m(port)
	{
		for (size_t i = 0; i < poolSize_m; ++i)
		{
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host_m + ":" + port_m, grpc::InsecureChannelCredentials());
			connections.emplace_back(VarifyService::NewStub(channel)); // �͵ش���gRPC������������ֵ����
		}
	};

	~RPConPool()
	{
		is_running_.store(false, std::memory_order::memory_order_release); // �������ӳز�������
		std::unique_lock<std::mutex> lock(mutex_);
		cond_var_.notify_all(); // ֪ͨ���еȴ����߳�
		while (!connections.empty())
		{
			connections.pop_back(); // ������ӳ��еĴ������
		}
	}

	std::unique_ptr<VarifyService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_var_.wait(lock, [this]
					   { return !connections.empty() || !is_running_.load(std::memory_order::memory_order_acquire); }); // �ȴ�ֱ���п������ӻ����ӳ�ֹͣ����
		if (!is_running_.load(std::memory_order::memory_order_acquire))
		{
			return nullptr; // ������ӳ���ֹͣ���У����ؿ�ָ��
		}
		auto connection = std::move(connections.front()); // ��ȡ���ӳ��е����һ���������
		connections.pop_back();							  // �����ӳ����Ƴ��ô������
		return connection;								  // ���ػ�ȡ�Ĵ������
	}

	void returnConnection(std::unique_ptr<VarifyService::Stub> connection)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (is_running_.load(std::memory_order::memory_order_acquire))
		{
			connections.push_back(std::move(connection)); // ���������Ż����ӳ�
			cond_var_.notify_one();						  // ֪ͨһ���ȴ����߳�
		}
		else
		{
			// ������ӳ���ֹͣ���У�������󽫱�����
			return;
		}
	}

private:
	std::atomic<bool> is_running_{true};						   // ԭ�Ӳ���ֵ����ʾ���ӳ��Ƿ���������
	size_t poolSize_m;											   // ���ӳش�С
	std::string host_m;											   // ������������ַ
	std::string port_m;											   // �������˿ں�
	std::vector<std::unique_ptr<VarifyService::Stub>> connections; // �洢���ӳ��е�gRPC�������
	std::mutex mutex_;											   // �����������ڱ������ӳصķ���
	std::condition_variable cond_var_;							   // ���������������̼߳�ͬ��
};

class VarfiyGrpcClient : public singleton<VarfiyGrpcClient>
{
	friend class singleton<VarfiyGrpcClient>;

public:
	GetVarifyRsp GetVarifyCode(const std::string &email)
	{
		ClientContext context;
		GetVarifyRsp response;
		GetVarifyReq request;
		request.set_email(email);
		auto stub = pool->getConnection();
		if (!stub)
		{
			std::cerr << "No available gRPC connection." << std::endl;
			LogSystem::getInstance()->error("No available gRPC connection.");// 记录错误日志
			// 这里可以选择抛出异常或返回一个默认的响应对象
			return GetVarifyRsp();
		}
		Status status = stub->GetVarifyCode(&context, request, &response);
		pool->returnConnection(std::move(stub)); // ���۳ɹ���񶼹黹����
		if (status.ok())
		{
			return response;
		}
		else
		{
			std::cerr << "RPC failed: " << status.error_message() << std::endl;
			LogSystem::getInstance()->error("RPC failed: " + status.error_message());// 记录错误日志
			return GetVarifyRsp();
		}
	}

private:
	VarfiyGrpcClient();
	// std::unique_ptr<VarifyService::Stub> stub_;// gRPC����൱��һ���ͻ��˴��������ڵ���Զ�̷���
	std::unique_ptr<RPConPool> pool;
};
#endif
