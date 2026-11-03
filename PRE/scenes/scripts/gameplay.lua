-- Gameplay-first example: code creates and controls entities at runtime.
-- Space creates/moves a marker; releasing Space removes it.
local counter = 0

function on_start()
    pre.spawn("marker", "sprite", 2, 0, 0)
end

function on_update(dt)
    local x, y, z = pre.position()
    local speed = 4
    if pre.action_down("move_left") then x = x - speed * dt end
    if pre.action_down("move_right") then x = x + speed * dt end
    if pre.action_down("move_up") then y = y + speed * dt end
    if pre.action_down("move_down") then y = y - speed * dt end
    pre.set_position(x, y, z)

    if pre.key_pressed("space") then
        counter = counter + 1
        if pre.entity_exists("marker") then
            pre.set_entity_position("marker", x + 1.5, y, 0)
        end
    end
    if not pre.key_down("space") and counter > 0 and pre.entity_exists("marker") then
        pre.destroy("marker")
    end
end
