-- Set the namespace
local ns = {}
setmetatable(ns, {__index = _G})
bat_drain_attack = ns
setfenv(1, ns)

-- local references
local attacker = nil
local target = nil
local target_actor = nil
local skill = nil

local attacker_pos_x = 0.0
local attacker_pos_y = 0.0

local distance_moved = 0.0
local start_x_position = 0.0
local start_y_position = 0.0
local jump_end_x_position = 0.0
local diff_x = 0.0
local diff_y = 0.0

local enemy_pos_x = 0.0
local enemy_pos_y = 0.0

local attack_step = 0

local damage_triggered = false

local Battle = nil
local fang_anim = nil
local fang_anim_time = 0
local fang_anim_started = false

-- attacker, the BattleActor attacking
-- target, the BattleActor target
-- The skill id used on target
function Initialize(_attacker, _target, _skill)
    -- Keep the reference in memory
    attacker = _attacker
    target = _target
    target_actor = _target:GetActor()
    skill = _skill

    -- Don't attack if the attacker isn't alive
    if (attacker:IsAlive() == false) then
        return
    end

    -- Get the current attackers' positions
    attacker_pos_x = attacker:GetXLocation()
    attacker_pos_y = attacker:GetYLocation()

    enemy_pos_x = target_actor:GetXLocation()
    enemy_pos_y = target_actor:GetYLocation()

    attack_step = 0

    damage_triggered = false

    distance_moved = SystemManager:GetUpdateTime() / vt_map.MapMode.NORMAL_SPEED * 100.0
    start_x_position = attacker_pos_x
    jump_end_x_position = attacker_pos_x - 140
    start_y_position = attacker_pos_y
    diff_x = attacker_pos_x - 70
    diff_y = attacker_pos_y - 50


    Battle = ModeManager:GetTop()
    -- A common claw slash animation
    fang_anim = Battle:CreateBattleAnimation("data/entities/battle/effects/fangs_crush.lua")
    fang_anim_time = 0
    fang_anim_started = false
end


function Update()
    -- The update time can vary, so update the distance on each update as well.
    distance_moved = SystemManager:GetUpdateTime() / vt_map.MapMode.NORMAL_SPEED * 100.0

    -- Start to jump slightly
    if (attack_step == 0) then
        attacker:ChangeSpriteAnimation("idle")
        AudioManager:PlaySound("data/sounds/throw.wav")

        attack_step = 1
    end
    -- Make the attacker jump
    if (attack_step == 1) then

        if (attacker_pos_x > diff_x) then
            attacker_pos_x = attacker_pos_x - distance_moved
            if attacker_pos_x < diff_x then attacker_pos_x = diff_x end
        end
        if (attacker_pos_x < diff_x) then
            attacker_pos_x = attacker_pos_x + distance_moved
            if attacker_pos_x > diff_x then attacker_pos_x = diff_x end
        end

        if (attacker_pos_y > diff_y) then
            attacker_pos_y = attacker_pos_y - distance_moved
            if attacker_pos_y < diff_y then attacker_pos_y = diff_y end
        end
        if (attacker_pos_y < diff_y) then
            attacker_pos_y = attacker_pos_y + distance_moved
            if attacker_pos_y > diff_y then attacker_pos_y = diff_y end
        end

        attacker:SetXLocation(attacker_pos_x)
        attacker:SetYLocation(attacker_pos_y)

        -- half-jump done
        if (attacker_pos_x == diff_x and attacker_pos_y == diff_y) then
            attack_step = 2
        end
    end

    -- Second half-jump
    if (attack_step == 2) then
        if (attacker_pos_x > jump_end_x_position) then
            attacker_pos_x = attacker_pos_x - distance_moved
            if attacker_pos_x < jump_end_x_position then attacker_pos_x = jump_end_x_position end
        end
        if (attacker_pos_x < jump_end_x_position) then
            attacker_pos_x = attacker_pos_x + distance_moved
            if attacker_pos_x > jump_end_x_position then attacker_pos_x = jump_end_x_position end
        end

        if (attacker_pos_y > start_y_position) then
            attacker_pos_y = attacker_pos_y - distance_moved
            if attacker_pos_y < start_y_position then attacker_pos_y = start_y_position end
        end
        if (attacker_pos_y < start_y_position) then
            attacker_pos_y = attacker_pos_y + distance_moved
            if attacker_pos_y > start_y_position then attacker_pos_y = start_y_position end
        end

        attacker:SetXLocation(attacker_pos_x)
        attacker:SetYLocation(attacker_pos_y)

        -- half-jump done
        if (attacker_pos_y == start_y_position) then
            Battle:TriggerBattleParticleEffect("data/visuals/particle_effects/dust.lua", attacker_pos_x, attacker_pos_y)

            -- Init the slash effect life time
            fang_anim_time = 0
            fang_anim_started = false

            attack_step = 3
        end
    end

    -- Wait for it to finish
    if (attack_step == 3) then
        fang_anim_time = fang_anim_time + SystemManager:GetUpdateTime()
        if (fang_anim_started == false) then
            fang_anim_started = true

            fang_anim:SetXLocation(target_actor:GetXLocation());
            fang_anim:SetYLocation(target_actor:GetYLocation() + 0.1) -- To see the effect
            fang_anim:SetVisible(true)
            fang_anim:Reset()
            Battle:TriggerBattleParticleEffect("data/visuals/particle_effects/hp_drain.lua",
                target_actor:GetXLocation(), target_actor:GetYLocation())

            -- Triggers the damage in the middle of the attack animation
            skill:ExecuteBattleFunction(attacker, target)
            -- Remove the skill points at the end of the attack
            attacker:SubtractSkillPoints(skill:GetSPRequired())
            attack_step = 4
        end
    end

    -- Return to start pos
    if (attack_step == 4) then
        fang_anim_time = fang_anim_time + SystemManager:GetUpdateTime()

        if (attacker_pos_x > start_x_position) then
            attacker_pos_x = attacker_pos_x - distance_moved
            if attacker_pos_x < start_x_position then attacker_pos_x = start_x_position end
        end
        if (attacker_pos_x < start_x_position) then
            attacker_pos_x = attacker_pos_x + distance_moved
            if attacker_pos_x > start_x_position then attacker_pos_x = start_x_position end
        end

        if (attacker_pos_y > start_y_position) then
            attacker_pos_y = attacker_pos_y - distance_moved
            if attacker_pos_y < start_y_position then attacker_pos_y = start_y_position end
        end
        if (attacker_pos_y < start_y_position) then
            attacker_pos_y = attacker_pos_y + distance_moved
            if attacker_pos_y > start_y_position then attacker_pos_y = start_y_position end
        end

        attacker:SetXLocation(attacker_pos_x)
        attacker:SetYLocation(attacker_pos_y)

        -- Stops once the animation is done
        if (fang_anim ~= nil and fang_anim_time > 75 * 4) then -- 300, 410 + 300 = 710 (< 730).
            fang_anim:SetVisible(false)
            fang_anim:Remove()
            -- The Remove() call will make the engine delete the object, so we set it to nil
            -- to avoid using it again.
            fang_anim = nil
            return true
        end
    end

    return false
end
