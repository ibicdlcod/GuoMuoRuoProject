#ifndef LUA_H
#define LUA_H

#include <QMap>
#include <QString>
#include <sol/sol.hpp>

using LuaMap = QMap<QString, int>;

namespace sol {
template <>
struct is_container<LuaMap> : std::true_type {};

template <>
struct usertype_container<LuaMap> {
    static int get(lua_State* L) {
        QString key = luaL_checkstring(L, 2);
        LuaMap *data = *static_cast<LuaMap **>(lua_touserdata(L, 1));
        if(!(*data).contains(key)) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushinteger(L, (*data)[key]);
        return 1;
    }
};

}

namespace LuaInit {
void init(sol::state &lua);
}

#endif // LUA_H
