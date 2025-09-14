#include "VarfiyGrpcClient.h"
VarfiyGrpcClient::VarfiyGrpcClient() {
	auto& g_config_mgr = ConfiMgr::getInstance();
	std::string host = g_config_mgr["VarifyServer"]["host"];
	std::string port = g_config_mgr["VarifyServer"]["port"]; // �������л�ȡgRPC�������������Ͷ˿�
	this->pool = std::make_unique<RPConPool>(4, host, port); // ����gRPC���ӳ�
}