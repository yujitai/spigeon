/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file map_def.h
 * @author yujitai
 * @date 2020/01/06 15:31:31
 ******************************************************************/

#ifndef  __MAP_DEF_H_
#define  __MAP_DEF_H_

// Map define.
#define MAP_HAS1(m,k)  ((bool) ((m).find((k))!=(m).end()))
#define MAP_HAS2(m,k1,k2)   (MAP_HAS1((m),(k1))?MAP_HAS1((m)[(k1)],(k2)):false)
#define FOR_MAP(m,K,V,i) for(std::map<K,V>::iterator i=(m).begin();i!=(m).end();i++)
#define FOR_UNORDERED_MAP(m,K,V,i) for(std::unordered_map<K,V>::iterator i=(m).begin();i!=(m).end();i++)
#define MAP_KEY(i) ((i)->first)
#define MAP_VAL(i) ((i)->second)
#define MAP_ERASE1(m,k) if(MAP_HAS1((m),(k))) (m).erase((k));
#define MAP_ERASE2(m,k1,k2) \
if(MAP_HAS1((m),(k1))){ \
    MAP_ERASE1((m)[(k1)],(k2)); \
    if((m)[(k1)].size()==0) \
        MAP_ERASE1((m),(k1)); \
}

#endif  //__MAP_DEF_H_


