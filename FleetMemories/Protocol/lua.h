#ifndef LUA_H
#define LUA_H

using LuaMap = QMap<QString, int>;

namespace sol {
template <>
struct is_container<LuaMap> : std::true_type {};

template <>
struct usertype_container<LuaMap> {

};
}

#endif // LUA_H
