#pragma once
#include <string>
namespace fl {
// Rendering data only; no sockets or authoritative state live in the UI.
struct ConnectionView {
    bool title=true,hideHud=false;
    bool menu=true,editingAddress=false,networked=false;
    std::string tableStatus="SHARED TABLE / HOST AUTHORITY";
    int side=0;
    std::string address="127.0.0.1",status="CHOOSE LOCAL, HOST, OR JOIN";
};
}
