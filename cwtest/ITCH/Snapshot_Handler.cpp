#include <LLIO/EPoll_Dispatcher.h>
#include <Logger/Logger.h>
#include <Util/Parameter.h>
#include <Util/Time.h>
#include <ITCH/Snapshot_Session.h>
#include <Data/Handler.h>
#include <vector>
#include <set>
#include <boost/function.hpp>
#include <app/app.h>
#include <Logger/Logger.h>
#include <Util/tbb_hash_util.h>
#include <tbb/concurrent_hash_map.h>
#include <map>


namespace hf_cwtest {
         using namespace hf_core;
         using namespace hf_tools;

Snapshot_Handler::Snapshot_Handler(Application* app):
        Handler(app, "Snapshot_Handler"){
    }
Snapshot_Handler::Snapshot_Handler(Application* app, const string& handler_type):
        Handler(app, handler_type){
    }
    

}

