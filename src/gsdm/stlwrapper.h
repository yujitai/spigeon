/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file stlwrapper.h
 * @author yujitai
 * @date 2020/01/08 12:48:30
 ******************************************************************/


#ifndef  __STLWRAPPER_H_
#define  __STLWRAPPER_H_

#define STR(x) ((x).c_str())

// Map define.
// #define MAP_TRY_GET(m,k,i) ( MAP_HAS1((m),(k)) ) 
#define MAP_HAS1(m,k)  ((bool)((m).find((k))!=(m).end()))
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

// Vector define.
#define FOR_VECTOR(v,i) for(uint32_t i=0;i<(v).size();i++)
#define FOR_VECTOR_ITERATOR(e,v,i) for(std::vector<e>::iterator i=(v).begin();i!=(v).end();i++)
#define FOR_VECTOR_WITH_START(v,i,s) for(uint32_t i=s;i<(v).size();i++)
#define ADD_VECTOR_END(v,i) (v).push_back((i))
#define ADD_VECTOR_BEGIN(v,i) (v).insert((v).begin(),(i))
#define VECTOR_VAL(i) (*(i))


#endif  //__STLWRAPPER_H_


