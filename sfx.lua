-- sfx.lua
-- A simple procedural audio synthesizer for generating game sound effects.

Sfx = {}
Sfx.__index = Sfx

local sample_rate = 44100
local PI_2 = math.pi * 2

-- Waveform generators
local function sine_wave(t, freq)
    return math.sin(t * freq * PI_2)
end

local function square_wave(t, freq)
    return math.sin(t * freq * PI_2) > 0 and 1 or -1
end

local function noise()
    return math.random() * 2 - 1
end

function Sfx:new()
    local class = self or Sfx
    local instance = setmetatable({}, class)
    instance.sounds = {}
    instance.sound_cache = {} -- Cache generated audio sources
    instance.muted = false
    instance:define_sounds()
    return instance
end

-- Define the 'recipes' for each sound effect
function Sfx:define_sounds()
    self.sounds.jump = {
        waveform = sine_wave,
        attack = 0.01,
        decay = 0.1,
        sustain = 0.1,
        release = 0.1,
        start_freq = 440,
        end_freq = 880,
        volume = 0.3
    }

    self.sounds.land = {
        waveform = noise,
        attack = 0.01,
        decay = 0.15,
        sustain = 0,
        release = 0.1,
        start_freq = 600,
        end_freq = 200,
        volume = 0.5
    }

    self.sounds.collect = {
        waveform = square_wave,
        attack = 0.05,
        decay = 0.1,
        sustain = 0.2,
        release = 0.2,
        start_freq = 523,
        end_freq = 1046,
        volume = 0.4
    }

    self.sounds.stomp = {
        waveform = noise,
        attack = 0.01,
        decay = 0.2,
        sustain = 0,
        release = 0.1,
        start_freq = 1200,
        end_freq = 100,
        volume = 0.6
    }

    self.sounds.shoot = {
        waveform = noise,
        attack = 0.01,
        decay = 0.05,
        sustain = 0,
        release = 0.05,
        start_freq = 1800,
        end_freq = 1200,
        volume = 0.3
    }

    self.sounds.hit_wall = {
        waveform = noise,
        attack = 0.01,
        decay = 0.1,
        sustain = 0,
        release = 0.1,
        start_freq = 800,
        end_freq = 400,
        volume = 0.4
    }

    self.sounds.hit_enemy = {
        waveform = sine_wave,
        attack = 0.01,
        decay = 0.2,
        sustain = 0,
        release = 0.2,
        start_freq = 600,
        end_freq = 200,
        volume = 0.5
    }

    self.sounds.shriek = {
        waveform = square_wave,
        attack = 0.02,
        decay = 0.15,
        sustain = 0.1,
        release = 0.15,
        start_freq = 1200,
        end_freq = 800,
        volume = 0.35
    }

    self.sounds.grapple = {
        waveform = sine_wave,
        attack = 0.01,
        decay = 0.05,
        sustain = 0.1,
        release = 0.1,
        start_freq = 200,
        end_freq = 600,
        volume = 0.3
    }

    self.sounds.dash = {
        waveform = square_wave,
        attack = 0.005,
        decay = 0.04,
        sustain = 0.03,
        release = 0.05,
        start_freq = 260,
        end_freq = 120,
        volume = 0.25
    }

    self.sounds.death = {
        waveform = noise,
        attack = 0.01,
        decay = 0.3,
        sustain = 0.1,
        release = 0.3,
        start_freq = 400,
        end_freq = 100,
        volume = 0.6
    }
end

function Sfx:play(name)
    if self.muted then
        return
    end

    local sound_data = self.sounds[name]
    if not sound_data then return end

    -- Check cache first
    if self.sound_cache[name] then
        local source = self.sound_cache[name]:clone()
        source:play()
        return
    end

    if type(sound_data) == "table" then
        -- Procedural sound generation (only once, then cached)
        local duration = (sound_data.attack or 0) + (sound_data.decay or 0) + (sound_data.sustain or 0) + (sound_data.release or 0)
        if duration == 0 then return end

        local samples = {}
        local sample_count = math.floor(sample_rate * duration)

        for i = 1, sample_count do
            local t = i / sample_rate
            local freq_t = t / duration
            local freq = sound_data.start_freq + (sound_data.end_freq - sound_data.start_freq) * freq_t

            -- Envelope
            local envelope = 0.0
            local attack_time = sound_data.attack or 0
            local decay_time = sound_data.decay or 0
            local sustain_level = sound_data.sustain_level or 0.5
            local sustain_time = sound_data.sustain or 0
            local release_time = sound_data.release or 0

            if t <= attack_time then
                envelope = attack_time > 0 and (t / attack_time) or 1.0
            elseif t <= attack_time + decay_time then
                envelope = 1.0 - (1.0 - sustain_level) * ((t - attack_time) / decay_time)
            elseif t <= attack_time + decay_time + sustain_time then
                envelope = sustain_level
            elseif t <= duration then
                envelope = release_time > 0 and (sustain_level * ((duration - t) / release_time)) or 0
            end

            samples[i] = sound_data.waveform(t, freq) * envelope * sound_data.volume
        end

        local soundData = love.sound.newSoundData(sample_count, sample_rate, 16, 1)
        for i = 1, sample_count do
            soundData:setSample(i - 1, samples[i] or 0)
        end
        local source = love.audio.newSource(soundData)
        source:setVolume(sound_data.volume)

        -- Cache the source for future use
        self.sound_cache[name] = source

        -- Play a clone so the cached version stays pristine
        local clone = source:clone()
        clone:play()
    elseif type(sound_data) == "Source" and not sound_data:isPlaying() then
        sound_data:play()
    end
end

function Sfx:set_muted(value)
    self.muted = value == true
    love.audio.setVolume(self.muted and 0 or 1)
end

function Sfx:stop(name)
    if self.sounds[name] and type(self.sounds[name]) == "Source" and self.sounds[name]:isPlaying() then
        self.sounds[name]:stop()
    end
end

function Sfx:update(_dt)
    -- This could be used for fading sounds, etc.
    if not self.sound_cache then
        return
    end
end

return Sfx
