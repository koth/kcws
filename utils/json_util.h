/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  json_util.h
 * Description:   description
 * Author:	Koth(Yaowen Chen)
 *
 */
#ifndef UTILS_JSON_UTIL_H_
#define UTILS_JSON_UTIL_H_
#include <stdint.h>
#include "jsonxx.h"

namespace json_util {

template <typename T>
T FromJsonValue(const jsonxx::Value& jval) {
    return jval.get<T>();
}

template <typename T>
bool ReadFromJson(const std::string& name, const jsonxx::Object& obj, T& val) {
    const std::map<std::string, jsonxx::Value*>& kvs=obj.kv_map();
    auto it=kvs.find(name);
    if(it==kvs.end()){
      return false;
    }
    val= FromJsonValue<T>(*(it->second));
    return true;
}

template <typename T>
bool ReadArray(std::string name, const jsonxx::Object& obj, std::vector<T>& rets) {
  if (!obj.has<jsonxx::Array>(name)) return false;
  jsonxx::Array arr = obj.get<jsonxx::Array>(name);
  const std::vector<jsonxx::Value*>& values=arr.values();
  for(size_t i=0;i<values.size();i++){
    jsonxx::Value& val=*values[i];
    rets.push_back(FromJsonValue<T>(val));
  }
  return true;
}

template <typename T>
jsonxx::Value ToJsonValue(const T& val) {
  return jsonxx::Value(val);
}

template <typename T>
void WriteToJson(const std::string& name,jsonxx::Object& obj,  const T& val) {
   obj<<name<<ToJsonValue<T>(val);
}

template <typename T>
bool WriteArray(const std::string& name, jsonxx::Object& obj, const std::vector<T>& rets) {
  jsonxx::Array arr;
  int nn=rets.size();
  for(int i=0;i<nn;i++){
      jsonxx::Value val=ToJsonValue<T>(rets[i]);
      arr<<val;
  }
  obj<<name<<arr;
  return true;
}

template<>
inline float FromJsonValue(const jsonxx::Value& jval) {
    return static_cast<float>(jval.get<jsonxx::Number>());
}
template<>
inline double FromJsonValue(const jsonxx::Value& jval) {
    return static_cast<double>(jval.get<jsonxx::Number>());
}
template<>
inline int32_t FromJsonValue(const jsonxx::Value& jval) {
    return static_cast<int32_t>(jval.get<jsonxx::Number>());
}
template<>
inline int64_t FromJsonValue(const jsonxx::Value& jval) {
    return static_cast<int64_t>(jval.get<jsonxx::Number>());
}

}

#endif  // UTILS_JSON_UTIL_H_
