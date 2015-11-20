#ifndef SRAGENT_H
#define SRAGENT_H
#include <map>
#include "smartrest.h"
#include "srqueue.h"
#include "srbootstrap.h"
#include "srintegrate.h"
#include "srtimer.h"

int readSrTemplate(const std::string &path, std::string &srv, std::string &srt);


/**
 *  \class AbstractMsgHandler sragent.h
 *  \brief Virtual abstract functor for SmartREST message callbacks.
 */
class AbstractMsgHandler
{
public:
        virtual ~AbstractMsgHandler() {}
        /**
         *  \brief SmartREST message handler interface.
         *  \param r the SmartREST message tokenized into an SrRecord.
         *  \param agent reference to the SrAgent instance.
         */
        virtual void operator()(SrRecord &r, SrAgent &agent) = 0;
};


/**
 *  \class SrAgent sragent.h
 *  \brief Main implementation of a device agent.
 *
 *  SrAgent implements a highly performant event-driven framework for scheduling
 *  all registered SrTimer instances and SmartREST message handlers. The agent
 *  controls the calling thread and runs silents in the background, until when
 *  a timeout event triggered by a SrTimer or a message with registered handler
 *  is received. The agent contains both an ingress and egress SrQueue, which
 *  are usually connected to an SrDevicePush and SrReporter for receiving
 *  responses and reporting requests.
 *
 *  Note the agent can not guarantee accurate timer scheduling, which is
 *  especially true when the system is under heavy load. The only guarantee is
 *  that a timer will not be scheduled before its intended fire time. Do NOT rely
 *  on the agent to perform real-time scheduling.
 */
class SrAgent
{
private:
        using string = std::string;
public:
        typedef uint16_t MsgID;
        /**
         *  \brief SrAgent constructor.
         *
         *  Note it is highly discouraged to instantiate more than one SrAgent
         *  instance.
         *
         *  \param _server server URL (with no trailing slash).
         *  \param deviceid unique device ID for registration to Cumulocity.
         *  \param igt pointer to your own integration instance.
         *  \param boot pointer to your own bootstrap instance.
         */
        SrAgent(const string &_server, const string &deviceid,
                SrIntegrate *igt = NULL, SrBootstrap *boot = NULL);
        virtual ~SrAgent();

        /**
         *  \brief Get the Cumulocity tenant this device is registered to.
         */
        const string &tenant() const {return _tenant;}
        /**
         *  \brief Get the username this device received from registration.
         */
        const string &username() const {return _username;}
        /**
         *  \brief Get the password this device received from registration.
         */
        const string &password() const {return _password;}
        /**
         *  \brief Get the encode64 encoded username and password (for
         *  HTTP basic authorization).
         */
        const string &auth() const {return _auth;}
        /**
         *  \brief Get the server URL.
         */
        const string &server() const {return _server;}
        /**
         *  \brief Get the unique device ID.
         */
        const string &deviceID() const {return did;}
        /**
         *  \brief Get the eXternal ID of the registered SmartREST template
         *  from the integration process.
         */
        const string &XID() const {return xid;}
        /**
         *  \brief Get the managed object ID Cumulocity assigned to the device.
         */
        const string &ID() const {return id;}
        /**
         *  \brief Perform the registration process.
         *  \param path the path to store received credentials.
         *  \return 0 on success, -1 otherwise.
         */
        int bootstrap(const string &path);
        /**
         *  \brief Perform the integration process.
         *  \param srv SmartREST template version number
         *  \param srt SmartREST template content.
         *  \return 0 on success, -1 otherwise.
         */
        int integrate(const string &srv, const string &srt);
        /**
         *  \brief Put news to egress queue for reporting.
         *  \param news the news to report.
         *  \return 0 on success, -1 otherwise.
         */
        int send(const SrNews &news);
        /**
         *  \brief Enter the agent loop.
         *
         *  This function takes over the calling thread, starts the agent for
         *  scheduling. Note this function does not return.
         */
        void loop();
        void addTimer(SrTimer &timer) {timers.push_back(&timer);}
        void addMsgHandler(MsgID msgid, AbstractMsgHandler *functor) {
                handlers[msgid] = functor;
        }

public:
        SrQueue<SrOpBatch> ingress;
        SrQueue<SrNews> egress;

private:
        typedef AbstractMsgHandler* _Callback;
        typedef std::map<MsgID, _Callback> _Handler;
        typedef std::vector<SrTimer*> _Timer;
        typedef _Timer::iterator _TimerIter;
        _Timer timers;
        _Handler handlers;
        string _tenant;
        string _username;
        string _password;
        string _auth;
        const string _server;
        const string did;
        string xid;
        string id;
        SrBootstrap *pboot;
        SrIntegrate *pigt;
};

#endif /* SRAGENT_H */
