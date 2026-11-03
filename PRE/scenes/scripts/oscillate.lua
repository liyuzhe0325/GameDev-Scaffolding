local origin_x, origin_y, origin_z
local elapsed = 0

function on_start()
    origin_x, origin_y, origin_z = pre.position()
end

function on_update(dt)
    elapsed = elapsed + dt
    pre.set_position(origin_x + math.sin(elapsed * 2) * 2, origin_y, origin_z)
end
