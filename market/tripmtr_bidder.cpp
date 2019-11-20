#include "tripmtr_bidder.h"

CLASS* tripmtr_bidder::oclass = NULL;

tripmtr_bidder::tripmtr_bidder(MODULE *module) {
    if (oclass==NULL)
    {
        oclass = gl_register_class(module,"tripmtr_bidder", sizeof(tripmtr_bidder), PC_BOTTOMUP|PC_AUTOLOCK);
        if (oclass==NULL) {
            GL_THROW("unable to register object class implemented by %s", __FILE__);
        }

        if (gl_publish_variable(oclass,
                PT_double, "bid_period[s]", PADDR(bid_period),
                PT_object, "market", PADDR(market),
                PT_enumeration, "role", PADDR(role),
                PT_KEYWORD, "BUYER", (enumeration) BUYER,
                PT_KEYWORD, "SELLER", (enumeration) SELLER,
                PT_double, "price", PADDR(price),
                PT_int64, "bid_id", PADDR(bid_id),
                PT_object, "triplex_meter", PADDR(triplex_meter),
                NULL)<1) {
            GL_THROW("unable to publish properties in %s",__FILE__);
        }

        memset(this, 0, sizeof(tripmtr_bidder));
    }
}

int tripmtr_bidder::create() {
    controller_bid.rebid = false;
    controller_bid.state = BS_UNKNOWN;
    controller_bid.bid_accepted = true;
    bid_id = -1;
    return SUCCESS;
}

int tripmtr_bidder::init(OBJECT *parent)
{
    new_bid_id = -1;
    next_t = 0;
    lastbid_id = -1;
    lastmkt_id = -1;
    OBJECT *hdr = OBJECTHDR(this);

    if(market == NULL) {
        throw "market is not defined";
    }

    thismkt_id = (int64*)gl_get_addr(market,"market_id");
    if(thismkt_id == NULL) {
        throw "market does not define market_id";
    }

    if(parent != NULL) {
        if(!gl_object_isa(parent, "triplex_meter")) {
            throw "tripmtr_bidder must be parented only to triplex_meters.";
        }

        triplex_meter_power = (double *) gl_get_addr(parent, "measured_real_power");
    } else {
        if(triplex_meter == NULL) {
            throw "Undefined triplex meter!";
        }

        triplex_meter_power = (double *) gl_get_addr(triplex_meter, "measured_real_power");
    }

    if(triplex_meter_power == NULL) {
        throw "Undefined triplex_meter's property: measured_real_power";
    }

    // find market's power unit
    gld_property *propMarketUnit;
    gld_string gstrMarketUnit;

    propMarketUnit = new gld_property(market, "unit");

    if(!propMarketUnit->is_valid()){
        throw "Cannot find unit property of market.";
    }

    gstrMarketUnit = propMarketUnit->get_string();

    if(gstrMarketUnit == "kW") {
        unit_multiplier = 0.001;
    } else if(gstrMarketUnit == "MW") {
        unit_multiplier = 0.000001;
    } else {
        unit_multiplier = 1.0;
    }

    char mktname[1024];
    if(bid_id == -1){
        controller_bid.bid_id = (int64)hdr->id;
        bid_id = (int64)hdr->id;
    } else {
        controller_bid.bid_id = bid_id;
    }

    submit = (FUNCTIONADDR)(gl_get_function(market, "submit_bid_state"));
    if(submit == NULL){
        gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *) gl_name(market, mktname, 1024));
        return 0;
    }

    return SUCCESS;
}

int tripmtr_bidder::isa(char *classname) {
    return strcmp(classname,"tripmtr_bidder") == 0;
}

TIMESTAMP tripmtr_bidder::sync(TIMESTAMP t0, TIMESTAMP t1) {
    OBJECT *hdr = OBJECTHDR(this);
    char mktname[1024];
    char ctrname[1024];

    if (t1==next_t || next_t==0) {
        next_t=t1+(TIMESTAMP)bid_period;
        if(lastmkt_id != *thismkt_id){
            controller_bid.rebid = false;
            lastmkt_id = *thismkt_id;
        }

        controller_bid.market_id = lastmkt_id;
        controller_bid.price = price;

        if(role == BUYER){
            controller_bid.quantity = *triplex_meter_power * unit_multiplier;
        } else {
            controller_bid.quantity = -(*triplex_meter_power) * unit_multiplier;
        }

        controller_bid.state = BS_UNKNOWN;
        ((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1023), (char *)gl_name(market, mktname, 1023), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));

        if(controller_bid.bid_accepted == false){
            return TS_INVALID;
        }

        controller_bid.rebid = true;
    }

    return next_t;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_tripmtr_bidder(OBJECT **obj, OBJECT *parent)
{
    try
    {
        *obj = gl_create_object(tripmtr_bidder::oclass);
        if (*obj!=NULL)
        {
            tripmtr_bidder *my = OBJECTDATA(*obj, tripmtr_bidder);
            gl_set_parent(*obj, parent);
            return my->create();
        }
    }
    catch (const char *msg)
    {
        gl_error("create_tripmtr_bidder: %s", msg);
        return 0;
    }
    return 1;
}

EXPORT int init_tripmtr_bidder(OBJECT *obj, OBJECT *parent)
{
    try
    {
        if (obj != NULL){
            return OBJECTDATA(obj, tripmtr_bidder)->init(parent);
        }
    }
    catch (const char *msg)
    {
        char name[64];
        gl_error("init_tripmtr_bidder(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
        return 0;
    }
    return 1;
}

EXPORT int isa_tripmtr_bidder(OBJECT *obj, char *classname)
{
    if(obj != 0 && classname != 0){
        return OBJECTDATA(obj, tripmtr_bidder)->isa(classname);
    } else {
        return 0;
    }
}

EXPORT TIMESTAMP sync_tripmtr_bidder(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
    TIMESTAMP t2 = TS_NEVER;
    tripmtr_bidder *my = OBJECTDATA(obj, tripmtr_bidder);
    switch (pass) {
    case PC_BOTTOMUP:
        t2 = my->sync(obj->clock,t1);
        obj->clock = t1;
        break;
    default:
        break;
    }
    return t2;
}
