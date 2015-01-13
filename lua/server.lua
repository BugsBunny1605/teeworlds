EPortalGun = EntityRegister("PortalGun", {"m_Pos"})
ECheckPoint = EntityRegister("CheckPoint", {"m_Pos", "m_Owner", "m_CaptureValue"})
ETurret = EntityRegister("Turret", {"m_Pos", "m_Team", "m_Health", "m_Ammo", "m_IsPowered"})

PortalGun = EntityCreate(CPortalGun)
PortalGun.m_Pos = vec2(100, 200)


function main()


end


function exit()


end

function update()


end
