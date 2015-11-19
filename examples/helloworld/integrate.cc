#include <srnethttp.h>
#include "integrate.h"
using namespace std;


int Integrate::integrate(const SrAgent &agent, const string &srv,
                         const string &srt)
{
        SrNetHttp http(agent.server()+"/s", srv, agent.auth());
        if (http.post("") < 0)
                return -1;

        SmartRest sr(http.response());
        SrRecord r = sr.next();
        if (r.size() && r[0].second == "40") {
                http.clear();
                if (http.post(srt) <= 0)
                        return -1;
                sr.reset(http.response());
                r = sr.next();
        }
        if (r.size() == 2 && r[0].second == "20") {
                xid = r[1].second;
                string s = "300," + agent.deviceID();
                http.clear();
                if (http.post(s) <= 0)
                        return -1;
                sr.reset(http.response());
                r = sr.next();
                if (r.size() && r[0].second == "50") {
                        http.clear();
                        if (http.post("301") <= 0)
                                return -1;
                        sr.reset(http.response());
                        r = sr.next();
                        if (r.size() == 3 && r[0].second == "801") {
                                id = r[2].second;
                                s = "302," + id + "," + agent.deviceID();
                                if (http.post(s) <= 0)
                                        return -1;
                                return 0;
                        }
                } else if (r.size() == 3 && r[0].second == "800") {
                        id = r[2].second;
                        return 0;
                }
        }
        return -1;
}