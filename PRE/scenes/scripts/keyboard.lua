function on_update(dt)
    local dx = (pre.key_down("d") and 1 or 0) - (pre.key_down("a") and 1 or 0)
    local dy = (pre.key_down("w") and 1 or 0) - (pre.key_down("s") and 1 or 0)
    local length = math.sqrt(dx * dx + dy * dy)
    if length == 0 then return end
    local x, y, z = pre.position()
    local step = 4 * dt / length
    pre.set_position(x + dx * step, y + dy * step, z)
end
