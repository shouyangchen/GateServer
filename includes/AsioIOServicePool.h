#ifndef ASIO_IO_SERVICE_POOL_H
#define ASIO_IO_SERVICE_POOL_H
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/signal_set.hpp>
#include "singleton.h"
#include<thread>

class AsioIOServicePool :public singleton<AsioIOServicePool>
{
	friend singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
#if BOOST_VERSION >= 107000
	using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
#else
	using Work = boost::asio::io_context::work;
#endif
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	// 使用 round-robin 的方式返回一个 io_service
	boost::asio::io_context& GetIOService();
	void Stop();
private:
	AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
	std::vector<IOService> _ioServices;
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;
	std::size_t                        _nextIOService;
};

#endif // ASIO_IO_SERVICE_POOL_H