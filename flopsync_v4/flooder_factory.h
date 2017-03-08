#ifndef FLOODER_WRAPPER_H
#define FLOODER_WRAPPER_H

#include "flooder_sync_node.h"
#include "flopsync2.h"

class FlooderFactory {
public:
    static inline FlooderSyncNode& instance(){
        static Synchronizer *sync=new Flopsync2; //The new FLOPSYNC, FLOPSYNC 2
        //Synchronizer* sync=new Flopsync1;
        static FlooderSyncNode flooder(sync,10000000000LL,2450,1,1);
        return flooder;
    }
    
private:
    FlooderFactory(){};
};

#endif /* FLOODER_WRAPPER_H */

