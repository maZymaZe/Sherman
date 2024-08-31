#ifndef __RAWMESSAGECONNECTION_H__
#define __RAWMESSAGECONNECTION_H__

#include "AbstractMessageConnection.h"
#include "GlobalAddress.h"
#include "Key.h"

#include <thread>
#include "SearchResult.h"

enum RpcType : uint8_t {
    MALLOC,
    FREE,
    NEW_ROOT,
    NOP,
    SEARCH,
    LEAFSTORE,
    INTERNALSTORE,
    LEAFDEL
};

struct RawMessage {
    RpcType type;

    uint16_t node_id;
    uint16_t app_id;

    GlobalAddress addr;  // for malloc

    int level;
    Key key;
    uint64_t page_addr;
    // uint64_t value;

    bool success;
    SearchResult sr;

} __attribute__((packed));

class RawMessageConnection : public AbstractMessageConnection {
   public:
    RawMessageConnection(RdmaContext& ctx, ibv_cq* cq, uint32_t messageNR);

    void initSend();
    void sendRawMessage(RawMessage* m, uint32_t remoteQPN, ibv_ah* ah);
};

#endif /* __RAWMESSAGECONNECTION_H__ */
