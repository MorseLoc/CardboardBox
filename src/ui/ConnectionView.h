#pragma once
#include <string>
namespace fl {
// Rendering data only; no sockets or authoritative state live in the UI.
struct ConnectionView {
    bool menu=true,editingAddress=false,networked=false;
    int side=0;
    std::string address="127.0.0.1",status="CHOOSE LOCAL, HOST, OR JOIN";
};
}
