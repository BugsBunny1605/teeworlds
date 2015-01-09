/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef ENGINE_SHARED_LUA_H
#define ENGINE_SHARED_LUA_H

#include <lua.hpp>
#include <base/system.h>
#include "lua_vec.h"

class CLua
{
private:
    CLuaVec2 m_Vec2;
    CLuaVec3 m_Vec3;
    CLuaVec4 m_Vec4;
protected:
    lua_State *m_pLua;
    char m_aFilename[256];

public:
    CLua();
    ~CLua();

    virtual bool LoadFile(const char *pFilename); //default loader

    virtual void ErrorHandler(char *pError); //error handler
    static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);
    static int Print(lua_State *L);

    //helper
    void PushString(const char *pString);
    void PushData(const char *pData, int Size);
    void PushInteger(int value);
    void PushFloat(float value);
    void PushBoolean(bool value);

    bool FunctionExist(const char *pFunctionName);
    void FunctionPrepare(const char *pFunctionName);
    int FunctionExec(const char *pFunctionName);
    int m_FunctionVarNum;

};

#endif
