
#include "game/item.h"
#include "game/db_service.h"
#include "game/game_event.h"

using namespace ff;
using namespace std;


DbService::DbService(){
    
}
DbService::~DbService(){
    
}

bool DbService::cleanup(){
    return true;
}

static void handleCreateEntityEvent(CreateEntityEvent& e){
    printf("handleEvent %d %s\n", e.eventID(), e.eventName().c_str());
}
static void handleUpdateEntityEvent(UpdateEntityEvent& e){
    printf("handleUpdateEntityEvent %d %s\n", e.eventID(), e.eventName().c_str());
}
static void handleDelEntityEvent(DelEntityEvent& e){
    printf("handleDelEntityEvent %d %s\n", e.eventID(), e.eventName().c_str());
}
bool DbService::init(){
    EVENT_BUS_LISTEN(&handleCreateEntityEvent);
    EVENT_BUS_LISTEN(&handleUpdateEntityEvent);
    EVENT_BUS_LISTEN(&handleDelEntityEvent);
    return true;
}

