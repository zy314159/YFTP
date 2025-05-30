#pragma once
#include "Singleton.hpp"
#include "FileUtil.hpp"
#include <queue>
#include <thread>
#include "Session.hpp"
#include <queue>
#include <map>
#include <functional>
#include "const.hpp"
#include <jsoncpp/json/json.h>

namespace Yftp
{
typedef function<void(shared_ptr<Session>, const short &msg_id, const string &msg_data)> FunCallBack;
class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void postMsgToQue(shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void dealMsg();
    void registerCallBacks();
    void helloWordCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void registerHandler(MSG_IDS msg_id, FunCallBack fun_call_back);
    void handleListCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleMkdirCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleRmdirCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleUploadCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleDownloadCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleCDCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handlePWDCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleCATCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void handleUserExitCallBack(shared_ptr<Session>, const short &msg_id, const string &msg_data);


    std::thread worker_thread_;
    std::queue<shared_ptr<LogicNode>> msg_que_;
    std::mutex mutex_;
    std::condition_variable consume_;
    bool b_stop_;
    std::map<short, FunCallBack> fun_callbacks_;
};

}