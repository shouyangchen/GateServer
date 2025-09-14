#include "const.h"
#include "singleton.h"
#include "ConfiMgr.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/support/status.h>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::StatusService;