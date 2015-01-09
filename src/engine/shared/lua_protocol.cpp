#include "lua_protocol.h"

void CLuaProtocol::Init()
{

}

void CLuaProtocol::SendRawPacket(CLuaPacket *pPacket)
{

}

bool CLuaProtocol::ProcessPacket(CLuaPacket *pPacket)
{
    //dint Type;
    //dint [Size]; //some packets have a static size
    const unsigned char *pBuffer = m_pBuffer;
    pBuffer = CVariableUInt64::Unpack(pBuffer, &pPacket->m_Type);
    pBuffer = CVariableUInt64::Unpack(pBuffer, &pPacket->m_Size);
}
