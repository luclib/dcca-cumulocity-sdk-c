#include <unistd.h>
#include <cstring>
#include "srreporter.h"
#define Q_OK SrQueue<SrNews>::Q_OK
using namespace std;


static void _insert(deque<string> &d, uint16_t cap, const string &s)
{
        if (d.size() >= cap) {
                const auto &ref = d.front();
                if (ref.compare(0, 3, "15,")) {
                        d.pop_front();
                } else {
                        const string front(ref);
                        d.erase(d.begin(), d.begin() + 2);
                        if (!d.empty() && d.front().compare(0, 3, "15,"))
                                d.emplace_front(front);
                }
        }
        d.emplace_back(s);
}


int SrReporter::start()
{
        int no = pthread_create(&tid, NULL, func, this);
        if (no) {
                srError("reporter: start failed, " + string(strerror(no)));
                return no;
        }
        srInfo("reporter: started.");
        return no;
}


void* SrReporter::func(void *arg)
{
        SrReporter *rep = (SrReporter*)arg;
        rep->http.setTimeout(20);
        auto &buf = rep->buffer;
        auto &cap = rep->_cap;
        string s;
        while (true) {
                s.clear();
                if (!rep->sleeping) {
                        for (const auto &item: buf) s += item + "\n";
                }
                // request aggregation
                string myxid;
                for (int j = 0;  j < SR_REPORTER_NUM; ++j) {
                        auto e = rep->out.get(SR_REPORTER_VAL);
                        if (e.second != Q_OK) break;
                        const string &data = e.first.data;
                        string cxid = rep->xid;
                        size_t pos = 0;
                        if (e.first.prio & SR_PRIO_XID) { // alternate XID
                                pos = data.find(',');
                                cxid = data.substr(0, pos++);
                        }
                        if (cxid != myxid) {
                                myxid = cxid;
                                s += "15," + myxid + "\n";
                                if (e.first.prio & SR_PRIO_BUF)
                                        _insert(buf, cap, "15,"+myxid);
                        }
                        s += data.substr(pos) + "\n";
                        if (e.first.prio & SR_PRIO_BUF) // request buffering
                                _insert(buf, cap, data.substr(pos));
                }
                if (rep->sleeping || s.empty()) continue;

                // exponential waiting
                int c = rep->http.post(s);
                for (uint32_t i = 0; c < 0 && i < SR_REPORTER_RETRIES;
                     ++i, c = rep->http.post(s)) {
                        ::sleep(1 << i);
                }
                if (c >= 0) {
                        buf.clear();
                        const string &resp = rep->http.response();
                        if (!resp.empty())
                                rep->in.put(SrOpBatch(resp));
                }
                rep->http.clear();
        }
        return NULL;
}
