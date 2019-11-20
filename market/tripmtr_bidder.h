#ifndef _tripmtr_bidder_h
#define _tripmtr_bidder_h

#include <stdarg.h>
#include "gridlabd.h"
#include "auction.h"

class tripmtr_bidder : public gld_object {
public:
    tripmtr_bidder(MODULE *);
    int create(void);
    int init(OBJECT *parent);
    int isa(char *classname);
    TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
    TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
    TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
    static CLASS *oclass;
public:
    double bid_period;
    OBJECT *market;
    typedef enum {
        BUYER=0,
        SELLER=1,
    } ROLE;
    enumeration role;
    double price;
    int64 *thismkt_id;
    int64 bid_id;
    BIDINFO controller_bid;
    OBJECT *triplex_meter;
private:
    int64 next_t;
    KEY new_bid_id;
    KEY lastbid_id;
    int64 lastmkt_id;
    FUNCTIONADDR submit;
    double *triplex_meter_power;
    double unit_multiplier;
};

#endif
