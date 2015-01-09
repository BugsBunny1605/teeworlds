function CCharacterCore()
    local self = {
        m_Pos = vec2(0, 0),
        m_Vel = vec2(0, 0),
        m_HookPos = vec2(0, 0),
        m_HookDir = vec2(0, 0),
        m_HookTick = 0,
        m_HookState = HOOK_IDLE,
        m_HookedPlayer = -1,
        m_Jumped = 0,
        m_TriggeredEvents = 0
    }
    return self
end


