/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef vnsw_agent_router_id_h
#define vnsw_agent_router_id_h

#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

#define XAPI_INTF_PREFIX "xapi"

namespace local = boost::asio::local;

class VnswRouteEvent {
public:
    enum Event {
        ADD_REQ,
        DEL_REQ,
        ADD_RESP,
        DEL_RESP,
        INTF_UP,
        INTF_DOWN
    };

    VnswRouteEvent(Ip4Address addr, Event event) : addr_(addr), event_(event) {};

    Ip4Address addr_;
    Event event_;
};

class VnswIfListener {
public:
    static const int kVnswRtmProto = 109;
    enum { max_buf_size = 4096 };
    VnswIfListener(boost::asio::io_service & io);
    virtual ~VnswIfListener() {
        if (read_buf_){
            delete [] read_buf_;
            read_buf_ = NULL;
        }
        delete revent_queue_;
    }
    
    static void Init (boost::asio::io_service &io) {
        assert(instance_ == NULL);
        instance_ = new VnswIfListener(io);
    }
    static void Shutdown() { 
        boost::system::error_code ec;
        if (instance_) {
            instance_->sock_.close(ec);
        }
    };

    void IntfNotify(DBTablePartBase *part, DBEntryBase *e);
    void KUpdateLinkLocalRoute(const Ip4Address &addr, bool del_rt);

    static VnswIfListener *GetInstance() {return instance_; }

private:
    typedef std::set<Ip4Address> Ip4HostTableType;
    class VnswIntfState : public DBState {
    public:
        VnswIntfState(Ip4Address addr) : addr_(addr) { };
        virtual ~VnswIntfState() { };
        Ip4Address GetIpAddr() { return addr_; };
    private:
        Ip4Address addr_;
    };

    void InitRouterId();
    void ReadHandler(const boost::system::error_code &, std::size_t length);
    void CreateVhostRoutes(Ip4Address &host_ip, uint8_t plen);
    void DeleteVhostRoutes(Ip4Address &host_ip, uint8_t plen);
    void SetupSocket();
    void RegisterAsyncHandler();
    void CreateSocket();
    uint32_t NetmaskToPrefix(uint32_t netmask);
    uint32_t FetchVhostAddress(bool netmask);
    void InitFetchLinks();
    void InitFetchRoutes();
    bool RouteEventProcess(VnswRouteEvent *re);
    void RouteHandler(struct nlmsghdr *nlh);
    void InterfaceHandler(struct nlmsghdr *nlh);
    void IfaddrHandler(struct nlmsghdr *nlh);
    int AddAttr(int type, void *data, int alen);
    int NlMsgDecode(struct nlmsghdr *nl, std::size_t len, uint32_t seq_no);

    static VnswIfListener *instance_;
    uint8_t *read_buf_;
    uint8_t tx_buf_[max_buf_size];

    int sock_fd_;
    local::datagram_protocol::socket sock_;
    bool ifaddr_listen_;
    DBTableBase::ListenerId intf_listener_id_;
    Ip4HostTableType    ll_addr_table_;
    WorkQueue<VnswRouteEvent *> *revent_queue_;
    int seqno_;
    bool vhost_intf_up_;
};

#endif
