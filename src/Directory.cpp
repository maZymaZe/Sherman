#include "Directory.h"
#include "Common.h"

#include <cstdlib>
#include <cstring>
#include "Connection.h"

#include "SearchResult.h"
#include "Tree.h"
// #include <gperftools/profiler.h>

GlobalAddress g_root_ptr = GlobalAddress::Null();
int g_root_level = -1;
bool enable_cache;

Directory::Directory(DirectoryConnection* dCon,
                     RemoteConnection* remoteInfo,
                     uint32_t machineNR,
                     uint16_t dirID,
                     uint16_t nodeID)
    : dCon(dCon),
      remoteInfo(remoteInfo),
      machineNR(machineNR),
      dirID(dirID),
      nodeID(nodeID),
      dirTh(nullptr) {
    {  // chunck alloctor
        GlobalAddress dsm_start;
        uint64_t per_directory_dsm_size = dCon->dsmSize / NR_DIRECTORY;
        dsm_start.nodeID = nodeID;
        dsm_start.offset = per_directory_dsm_size * dirID;
        chunckAlloc = new GlobalAllocator(dsm_start, per_directory_dsm_size);
    }

    dirTh = new std::thread(&Directory::dirThread, this);
}

Directory::~Directory() {
    delete chunckAlloc;
}

void Directory::dirThread() {
    bindCore((CPU_PHYSICAL_CORE_NUM - 1 - dirID) * 2 +
             1);  // bind to the last CPU core
    Debug::notifyInfo("dir %d launch!\n", dirID);

    while (true) {
        struct ibv_wc wc;
        pollWithCQ(dCon->cq, 1, &wc);

        switch (int(wc.opcode)) {
            case IBV_WC_RECV:  // control message
            {
                auto* m = (RawMessage*)dCon->message->getMessage();

                process_message(m);

                break;
            }
            case IBV_WC_RDMA_WRITE: {
                break;
            }
            case IBV_WC_RECV_RDMA_WITH_IMM: {
                break;
            }
            default:
                assert(false);
        }
    }
}

bool Directory::rpc_page_search(uint64_t page_addr,
                                const Key& k,
                                SearchResult& result) {
    char page_buffer[kkPageSize + 3];
    int counter = 0;
    page_addr+=(uint64_t)(dCon->dsmPool);
re_copy:
    if (++counter > 100) {
        printf("re read too many times\n");
        sleep(1);
    }
    memcpy(page_buffer, (void*)page_addr, kkPageSize);
    assert(STRUCT_OFFSET(LeafPage, hdr) == STRUCT_OFFSET(InternalPage, hdr));
    auto header = (Header*)(page_buffer + (STRUCT_OFFSET(LeafPage, hdr)));
    memset(&result, 0, sizeof(result));
    result.is_leaf = header->leftmost_ptr == GlobalAddress::Null();
    result.level = header->level;
    if (result.is_leaf) {
        LeafPage* page = (LeafPage*)page_buffer;
        if (!page->check_consistent()) {
            goto re_copy;
        }
        if (result.level != 0) {
            return false;
        }
        // never stale
        if (k >= page->hdr.highest) {  // should turn right
            result.slibing = page->hdr.sibling_ptr;
            return true;
        }
        if (k < page->hdr.lowest) {
            // assert(false);
            return false;
        }
        for (int i = 0; i < kLeafCardinality; ++i) {
            auto& r = page->records[i];
            if (r.key == k && r.value != kValueNull &&
                r.f_version == r.r_version) {
                result.val = r.value;
                break;
            }
        }
    } else {
        auto page = (InternalPage*)page_buffer;
        if (!page->check_consistent()) {
            goto re_copy;
        }
        if (result.level == 0)
            return false;
        if (k >= page->hdr.highest) {
            result.slibing = page->hdr.sibling_ptr;
            return true;
        }
        if (k < page->hdr.lowest) {
            return false;
        }
        assert(k >= page->hdr.lowest);
        if (k >= page->hdr.highest) {
            result.slibing = page->hdr.sibling_ptr;
            return true;
        }
        assert(k < page->hdr.highest);

        auto cnt = page->hdr.last_index + 1;
        if (k < page->records[0].key) {
            result.next_level = page->hdr.leftmost_ptr;
            return true;
        }

        for (int i = 1; i < cnt; ++i) {
            if (k < page->records[i].key) {
                result.next_level = page->records[i - 1].ptr;
                return true;
            }
        }
        result.next_level = page->records[cnt - 1].ptr;
    }
    return true;
}
void Directory::process_message(const RawMessage* m) {
    RawMessage* send = nullptr;
    switch (m->type) {
        case RpcType::MALLOC: {
            send = (RawMessage*)dCon->message->getSendPool();

            send->addr = chunckAlloc->alloc_chunck();
            break;
        }

        case RpcType::NEW_ROOT: {
            if (g_root_level < m->level) {
                g_root_ptr = m->addr;
                g_root_level = m->level;
#ifdef TREE_ENABLE_CACHE
                if (g_root_level >= 3) {
                    enable_cache = true;
                }
#endif
            }

            break;
        }

        case RpcType::SEARCH: {
            send = (RawMessage*)dCon->message->getSendPool();
            SearchResult tmp;
            send->success = rpc_page_search(m->page_addr, m->key, tmp);
            send->sr = tmp;
            break;
        }
        default:
            assert(false);
    }

    if (send) {
        dCon->sendMessage2App(send, m->node_id, m->app_id);
    }
}
