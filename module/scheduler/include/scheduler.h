#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#include <boost/make_unique.hpp>
#include <cstdlib>
#include <iostream>
#include "message.hpp"
#include "Environment.h"
#include "JsonSerializable.h"
#include "HdtnConfig.h"
#include "reg.hpp"

typedef std::unique_ptr<boost::asio::deadline_timer> SmartDeadlineTimer;
struct contactPlan_t {
    int contact;
    int source; 
    int dest; 
    int id;
    int start;
    int end;
    int rate; 
};
typedef std::vector<contactPlan_t> contactPlanVector_t;

class Scheduler {
public:
    static const std::string DEFAULT_FILE;
    Scheduler();
    ~Scheduler();
    int ProcessContactsFile(std::string jsonEventFileName);
    int ProcessComandLine(int argc, const char *argv[], std::string& jsonEventFileName);
    static std::string GetFullyQualifiedFilename(std::string filename) {
        return (Environment::GetPathHdtnSourceRoot() / "module/scheduler/src/").string() + filename;
    }
    volatile bool m_timersFinished;
private:
    void ProcessLinkUp(const boost::system::error_code&, int id, std::string event, zmq::socket_t * ptrSocket);
    void ProcessLinkDown(const boost::system::error_code&, int id, std::string event, zmq::socket_t * ptrSocket);

    HdtnConfig m_hdtnConfig;
};

#endif // SCHEDULER_H
