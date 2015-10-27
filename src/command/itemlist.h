#ifndef __COMMAND_ITEMLIST_H_
#define __COMMAND_ITEMLIST_H_

#include <list>
#include "db/db_define.h"
#include "util/utils.h"
#include "db/request.h"
#include "engine/engine.h"
#include "command/list.pb.h"
#include "util/slice.h"

namespace store{
struct Item{
    std::string member;
    std::string extra;
};

#define RETURN_IF_ENGINE_ERROR(op, ret)                                   \
    if ((ret) == ENGINE_ERROR) {                                          \
        log_fatal("engine fatal error");                                  \
        return Status::InternalError(op, "engine maybe corrupted");       \
    }

#define RETURN_IF_NOTFOUND(key, ret)                                      \
    if ((ret) == ENGINE_NOTFOUND) {                                       \
        return Status::NotFound(key);                                     \
    }

#define STORE_MAX_DELTA_TIME (10*365*24*3600)
uint32_t _list_sanitize_time(int32_t seconds) {
    if (seconds == 0) {
        return 0;
    } else if (seconds < 0) {
        return time(NULL);
    } else if (seconds < STORE_MAX_DELTA_TIME) {
        return time(NULL) + seconds;
    } else {
        return seconds;
    }
}

class ItemList{

protected:
    std::list<Item> _list;

public:
    ItemList(){}

    ~ItemList() {}

    int32_t size(){
        return _list.size();
    }

    std::string get_member(int32_t idx){
        std::list<Item>::iterator it = _list.begin();
        while(idx > 0){
            it++;
            idx--;
        }
        return it->member;
    }

    std::string get_extra(int32_t idx){
        std::list<Item>::iterator it = _list.begin();
        while(idx > 0){
            it++;
            idx--;
        }
        return it->extra;
    }

    Status get_store_data(Engine *engine,
            Slice _key,
            bool empty_is_reasonable = true
            ){
        std::string content_str;
        int32_t ret = engine->get(_key, &content_str);
        RETURN_IF_ENGINE_ERROR("get", ret);
        if(ret != ENGINE_NOTFOUND){
            Slice value_slice(content_str); 
            ValueHeader* value_header = (ValueHeader*)value_slice.data();
            if(value_header->type != ValueHeader::LIST_PB_TYPE){
                log_warning("data type incorrect. key:%s", _key.data());
                return Status::UnsupportedType("data_type", "type incorrect");
            }
            value_slice.remove_prefix(sizeof(ValueHeader));
            content_str = value_slice.to_string();
            store_list::list store_to_list; 
            store_to_list.ParseFromString(content_str); 
            load_from_protobuf(store_to_list);
        }else{
            if(empty_is_reasonable == false){
                log_warning("is not found. key:%s", _key.data());
                RETURN_IF_NOTFOUND(_key, ret);
            }
        }
        return Status::OK();
    }

    Status store_data(Engine *engine,
            uint64_t binlog_id,
            Slice _key,
            int32_t _seconds
            ){
        store_list::list list_to_store;
        store_to_protobuf(list_to_store);
        std::string store_value;
        list_to_store.SerializeToString(&store_value);
        size_t buf_len = sizeof(ValueHeader) + store_value.size();
        char *buf = (char*)zmalloc(buf_len);
        BufferHolder buf_holder(buf);
        memset(buf, 0, sizeof(ValueHeader));
        ValueHeader* value_header = (ValueHeader*)buf;
        value_header->type = ValueHeader::LIST_PB_TYPE;
        value_header->binlog_id = binlog_id;
        memcpy(buf + sizeof(ValueHeader), store_value.c_str(),
                store_value.size());
        Slice value(buf, buf_len);
        int32_t ret = engine->set(_key, value, _list_sanitize_time(_seconds));
        RETURN_IF_ENGINE_ERROR("set", ret);
        return Status::OK();
    }

    void load_from_protobuf(store_list::list store_to_list){
        Item node;
        for(int32_t idx = 0; idx < store_to_list.itemlist_size(); idx++){
            store_list::list::item item = store_to_list.itemlist(idx);
            node.member = item.member();
            node.extra = item.extra();
            _list.push_back(node);
        }
    }

    void store_to_protobuf(store_list::list &list_to_store ){
        std::list<Item>::iterator it = _list.begin();
        for(it = _list.begin();it != _list.end(); it++){
            store_list::list::item* item_t = list_to_store.add_itemlist();
            item_t->set_member(it->member);
            item_t->set_extra(it->extra);
        }
    }

    int32_t lpush(Item point){
        _list.push_front(point);
        return _list.size();
    }

    int32_t rpush(Item point){
        _list.push_back(point);
        return _list.size();
    }

    bool lsetbymember(Item point){
        for(std::list<Item>::iterator it = _list.begin(); it != _list.end(); it++){
            if(point.member == it->member){
                it->extra = point.extra;
                return true;
            }
        }
        return false;
    }

    bool lset(Item point,int index){
        if(index >= 0){
            if(index >= (int32_t)_list.size()){
                return false;
            }
            std::list<Item>::iterator it = _list.begin();
            while(index > 0){
                it++;
                index--;
            }
            *it = point;
        }else{
            index *= -1;
            if(index > (int32_t)_list.size()){
                return false;
            }
            std::list<Item>::iterator it = _list.begin();
            index = _list.size() - index;
            while(index > 0){
                it++;
                index--;
            }
            *it = point;
        }
        return true;
    }

    bool lpop(Item &point){
        if(_list.size() == 0)return false;
        std::list<Item>::iterator it = _list.begin();
        point = *it;
        _list.pop_front();
        return true;
    }

    bool rpop(Item &point){
        if(_list.size() == 0)return false;
        std::list<Item>::iterator it = _list.end();
        it--;
        point = *it;
        _list.pop_back();
        return true;
    }

    int32_t lrem(Item point , int32_t count){
        int32_t idx, delete_num = 0;
        std::list<Item>::iterator it = _list.begin();
        if(count < 0){
            count *= -1;
            it = _list.end();
            it--;
            for(idx = 0; idx < count; idx++){
                if(it->member == point.member){
                    delete_num++;
                    if(it == _list.begin()){
                        _list.erase(it);
                        break;
                    }else{
                        _list.erase(it--);
                    }
                }else{
                    if(it == _list.begin())break;
                    it--;
                }
            }
        }else{
            std::list<Item>::iterator it = _list.begin();
            if(count == 0)count = _list.size();
            for(idx = 0; idx < count && it != _list.end(); idx++){
                if(it->member == point.member){
                    delete_num++;
                    _list.erase(it++);
                }else{
                    it++;
                }
            }
        }
        return delete_num;
    }

    bool lrim(int32_t start, int32_t stop){
        if(start < 0){
            start = (int32_t)_list.size() + start;
        }
        if(start < 0){
            start = 0;
        }
        if(stop < 0){
            stop = (int32_t)_list.size() + stop;
        }
        if(start > stop)_list.clear();
        std::list<Item>::iterator it = _list.begin();
        for(int32_t idx = 0,list_len = _list.size(); idx < list_len; idx++){
            if(idx < start || idx >= stop){
                _list.erase(it++);
            }else{
                it++;
            }
        }
        if(_list.size() > 0){
            return true;
        }
        return false;
    }
};

}








#endif
