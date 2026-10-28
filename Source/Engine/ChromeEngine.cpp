#include "ChromeEngine.h"

namespace chaz
{

void ChromeEngine::prepare (double sampleRate, int /*block*/) noexcept
{
    sr = sampleRate > 1.0 ? sampleRate : 44100.0;
    invSr = 1.0f / (float) sr;
    heatSm.prepare (sr, 0.03);
    spaceSm.prepare (sr, 0.04);
    widthSm.prepare (sr, 0.03);
    edgeSm.prepare (sr, 0.025);
    chaosSm.prepare (sr, 0.05);
    masterSm.prepare (sr, 0.02);
    cutSm.prepare (sr, 0.025);
    reset();
    applyMacroTargets();
}

void ChromeEngine::reset() noexcept
{
    for (auto& v : voices)
    {
        v = Voice{};
        v.whLp.reset(); v.whHp.reset();
        v.flLp.reset(); v.plLp.reset();
        v.voiceLp.reset();
    }
    masterLpL.reset(); masterLpR.reset();
    edgeHpL.reset(); edgeHpR.reset();
    delayL.fill (0); delayR.fill (0);
    delayPos = 0;
    meter = 0;
    lastNote = -1;
    heldSize = 0;
    heldNotes.fill (-1);
}

int ChromeEngine::getActiveVoices() const noexcept
{
    int n = 0;
    for (const auto& v : voices) if (v.active) ++n;
    return n;
}

void ChromeEngine::applyMacroTargets() noexcept
{
    const float edge = clamp01 (params.edge);
    const float cutBase = clamp01 (params.cutoff);
    // Edge pushes cutoff open; heat doesn't touch filter here
    const float cutNorm = clamp01 (cutBase * (0.55f + edge * 0.55f));
    const float cutHz = 220.0f * std::pow (90.0f, cutNorm); // ~220..~20k
    masterLpL.setCutoff (cutHz, sr);
    masterLpR.setCutoff (cutHz, sr);
    cutSm.setTarget (cutNorm);

    edgeHpL.setCutoff (1800.0f + edge * 5500.0f, sr);
    edgeHpR.setCutoff (1800.0f + edge * 5500.0f, sr);

    heatSm.setTarget (clamp01 (params.heat));
    spaceSm.setTarget (clamp01 (params.space));
    widthSm.setTarget (clamp01 (params.width));
    edgeSm.setTarget (edge);
    chaosSm.setTarget (clamp01 (params.chaos));
    masterSm.setTarget (dbToGain (params.masterGainDb));
}

float ChromeEngine::noise() noexcept
{
    rng = rng * 1664525u + 1013904223u;
    return ((rng >> 8) & 0xffff) / 65535.0f * 2.0f - 1.0f;
}

ChromeEngine::Voice* ChromeEngine::allocVoice (int note, bool preferStealSame) noexcept
{
    if (preferStealSame)
        for (auto& v : voices)
            if (v.active && v.note == note)
                return &v;

    Voice* free = nullptr;
    Voice* oldest = &voices[0];
    for (auto& v : voices)
    {
        if (! v.active) { free = &v; break; }
        if (v.age > oldest->age) oldest = &v;
    }
    if (free) return free;
    for (auto& v : voices)
        if (v.note == note) return &v;
    return oldest;
}

ChromeEngine::Voice* ChromeEngine::findVoice (int note) noexcept
{
    for (auto& v : voices)
        if (v.active && v.note == note && ! v.releasing)
            return &v;
    for (auto& v : voices)
        if (v.active && v.note == note)
            return &v;
    return nullptr;
}

void ChromeEngine::startVoice (Voice& v, int note, float velocity, bool legato) noexcept
{
    v.active = true;
    v.note = note;
    v.targetNote = note;
    v.vel = 0.22f + 0.78f * clamp01 (velocity);
    v.age = 0;
    v.releasing = false;
    v.legato = legato;

    const float bendSemi = params.pitchBend * std::max (0.0f, params.bendRange);
    v.targetHz = midiToHz ((float) note + bendSemi);

    if (! legato || v.currentHz < 1.0f)
    {
        v.currentHz = v.targetHz;
        v.ampEnv = 0;
        v.ampStage = 0;
        v.whPhase = 0; v.vibPhase = 0;
        v.flPhase = 0; v.flEnv = 0; v.flStage = 0;
        v.plPhase = 0; v.plMod = 0;
        v.plEnv = 1.0f; v.plClick = 1.0f;
        v.bellPhase = 0; v.bellMod = 0; v.bellEnv = 1.0f;
        v.whLp.reset(); v.whHp.reset();
        v.flLp.reset(); v.plLp.reset();
        v.voiceLp.reset();
        v.chaosPhase = noise() * (float) M_PI;
        v.chaosAmt = (noise() * 0.5f + 0.5f);
    }
    else
    {
        // legato: keep amp, retarget pitch; soft re-pluck
        v.plEnv = std::max (v.plEnv, 0.55f);
        v.plClick = 0.65f;
        v.bellEnv = std::max (v.bellEnv, 0.35f);
        if (v.ampStage == 3) { v.ampStage = 1; }
        if (v.ampEnv < 0.2f) { v.ampEnv = 0.2f; v.ampStage = 0; }
    }

    const float hz = v.targetHz;
    v.whLp.setCutoff (hz * (2.0f + clamp01 (params.whistleTone) * 8.0f), sr);
    v.whHp.setCutoff (hz * 0.55f, sr);
    v.flLp.setCutoff (lerp (500.0f, 4200.0f, clamp01 (params.fluteTone)), sr);
    v.plLp.setCutoff (hz * (3.0f + clamp01 (params.pluckTone) * 10.0f), sr);
    v.voiceLp.setCutoff (hz * (2.5f + clamp01 (params.cutoff) * 9.0f), sr);
}

void ChromeEngine::noteOn (int midiNote, float velocity) noexcept
{
    velocity = clamp01 (velocity);

    // track held notes for mono re-trigger
    if (heldSize < 128)
    {
        bool already = false;
        for (int i = 0; i < heldSize; ++i)
            if (heldNotes[(size_t) i] == midiNote) { already = true; break; }
        if (! already)
            heldNotes[(size_t) heldSize++] = midiNote;
    }

    if (params.monoLegato)
    {
        Voice* live = nullptr;
        for (auto& v : voices)
            if (v.active && ! v.releasing) { live = &v; break; }
        if (! live)
            for (auto& v : voices)
                if (v.active) { live = &v; break; }

        if (live)
        {
            startVoice (*live, midiNote, velocity, true);
            lastNote = midiNote;
            return;
        }
        auto* v = allocVoice (midiNote, false);
        if (! v) return;
        // kill others in mono
        for (auto& o : voices)
            if (&o != v && o.active)
                o.releasing = true;
        startVoice (*v, midiNote, velocity, false);
        lastNote = midiNote;
        return;
    }

    auto* v = allocVoice (midiNote, true);
    if (! v) return;
    startVoice (*v, midiNote, velocity, false);
    lastNote = midiNote;
}

void ChromeEngine::noteOff (int midiNote) noexcept
{
    // remove from held stack
    for (int i = 0; i < heldSize; ++i)
    {
        if (heldNotes[(size_t) i] == midiNote)
        {
            for (int j = i; j < heldSize - 1; ++j)
                heldNotes[(size_t) j] = heldNotes[(size_t) (j + 1)];
            --heldSize;
            break;
        }
    }

    if (params.monoLegato && heldSize > 0)
    {
        const int next = heldNotes[(size_t) (heldSize - 1)];
        for (auto& v : voices)
        {
            if (v.active && ! v.releasing)
            {
                startVoice (v, next, v.vel, true);
                lastNote = next;
                return;
            }
        }
    }

    for (auto& v : voices)
        if (v.active && v.note == midiNote)
            v.releasing = true;
}

void ChromeEngine::allNotesOff() noexcept
{
    for (auto& v : voices)
        if (v.active)
            v.releasing = true;
    heldSize = 0;
}

static void advanceAdsr (float& env, int& stage, bool releasing,
                         float A, float D, float S, float R, float invSr, bool& alive) noexcept
{
    A = std::max (0.0008f, A);
    D = std::max (0.008f, D);
    R = std::max (0.008f, R);
    S = clamp01 (S);

    if (releasing && stage < 3)
        stage = 3;

    if (stage == 0)
    {
        env += invSr / A;
        if (env >= 1.0f) { env = 1.0f; stage = 1; }
    }
    else if (stage == 1)
    {
        env -= (1.0f - S) * invSr / D;
        if (env <= S) { env = S; stage = 2; }
    }
    else if (stage == 2)
    {
        env = S;
    }
    else
    {
        env -= invSr / R;
        if (env <= 0.0001f) { env = 0; alive = false; }
    }
}

void ChromeEngine::renderVoice (Voice& v, float& L, float& R) noexcept
{
    ++v.age;

    // Pitch bend + portamento
    const float bendSemi = params.pitchBend * std::max (0.0f, params.bendRange);
    v.targetHz = midiToHz ((float) v.targetNote + bendSemi);

    const float portaSec = clamp01 (params.portaTime) * 0.85f; // 0..850ms
    if (portaSec < 0.001f || ! params.monoLegato)
    {
        // poly: mild glide only if porta > 0
        if (portaSec < 0.001f)
            v.currentHz = v.targetHz;
        else
        {
            const float coeff = 1.0f - std::exp (-invSr / portaSec);
            v.currentHz += (v.targetHz - v.currentHz) * coeff;
        }
    }
    else
    {
        const float coeff = 1.0f - std::exp (-invSr / std::max (0.001f, portaSec));
        v.currentHz += (v.targetHz - v.currentHz) * coeff;
    }

    // Chaos: subtle pitch wander
    const float chaos = clamp01 (params.chaos);
    v.chaosPhase += (0.7f + v.chaosAmt * 1.8f) * 2.0f * (float) M_PI * invSr;
    const float chaosDet = chaos * 0.035f * std::sin (v.chaosPhase) * v.chaosAmt;
    const float hz = std::max (20.0f, v.currentHz * (1.0f + chaosDet));
    const double w = 2.0 * M_PI * (double) hz * (double) invSr;

    bool ampAlive = true;
    advanceAdsr (v.ampEnv, v.ampStage, v.releasing,
                 params.ampAttack, params.ampDecay, params.ampSustain, params.ampRelease,
                 invSr, ampAlive);

    // Pluck / bell oneshots can outlive sustain briefly
    if (! ampAlive && v.plEnv < 0.001f && v.bellEnv < 0.001f)
    {
        v.active = false;
        return;
    }

    float outL = 0, outR = 0;
    const float amp = v.ampEnv * v.vel;

    // ========== Whistle / air lead ==========
    if (params.whistleOn && amp > 0.0001f)
    {
        v.vibPhase += 2.0f * (float) M_PI * std::max (0.1f, params.whistleVibRate) * invSr;
        const float vib = 1.0f + clamp01 (params.whistleVib) * 0.012f * std::sin (v.vibPhase);
        v.whPhase += w * (double) vib;

        float sine = fastSin (v.whPhase);
        // soft 2nd for body
        sine = sine * 0.88f + 0.12f * fastSin (v.whPhase * 2.0);

        float breath = noise();
        breath = v.whHp.process (breath);
        breath *= clamp01 (params.whistleBreath) * (0.35f + 0.65f * amp);

        float wh = sine * (1.0f - clamp01 (params.whistleBreath) * 0.25f) + breath * 0.55f;
        wh = v.whLp.process (wh);
        // mild air shimmer
        wh += 0.08f * fastSin (v.whPhase * 3.01) * clamp01 (params.whistleTone) * amp;

        wh *= amp * 0.55f;
        const float g = dbToGain (params.whistleGainDb);
        // slight stereo from vib
        const float pan = 0.02f * std::sin (v.vibPhase * 0.5f);
        outL += wh * g * (1.0f - pan);
        outR += wh * g * (1.0f + pan);
    }

    // ========== Dark flute / wood ==========
    if (params.fluteOn && amp > 0.0001f)
    {
        bool flAlive = true;
        advanceAdsr (v.flEnv, v.flStage, v.releasing,
                     std::max (0.005f, params.fluteAttack),
                     0.25f, 0.75f, std::max (0.05f, params.ampRelease),
                     invSr, flAlive);

        v.flPhase += w;
        float ph01 = (float) std::fmod (v.flPhase / (2.0 * M_PI) + 8.0, 1.0);
        float pulse = ph01 < (0.25f + clamp01 (params.flutePulse) * 0.35f) ? 1.0f : -1.0f;
        float sine = fastSin (v.flPhase);
        float fl = lerp (sine, softClip (pulse * 0.9f), clamp01 (params.flutePulse));
        fl = v.flLp.process (fl);

        float wood = noise() * clamp01 (params.fluteNoise) * 0.25f;
        wood *= (0.4f + 0.6f * v.flEnv);
        fl = fl * 0.85f + wood;

        fl *= v.flEnv * v.vel * 0.48f;
        const float g = dbToGain (params.fluteGainDb);
        outL += fl * g * 0.97f;
        outR += fl * g * 1.03f;
    }

    // ========== Chrome pluck ==========
    if (params.pluckOn && v.plEnv > 0.0001f)
    {
        const float dec = std::max (0.04f, params.pluckDecay);
        v.plEnv *= std::exp (-invSr * (2.8f / dec));
        v.plClick *= std::exp (-invSr * (18.0f + clamp01 (params.pluckSharp) * 40.0f));

        const float fm = clamp01 (params.pluckBody);
        v.plMod += w * (2.0 + fm * 3.5);
        const float mod = fastSin (v.plMod) * (1.5f + fm * 4.0f) * v.plEnv;
        v.plPhase += w + (double) (mod * fm * 0.4f);

        float body = fastSin (v.plPhase);
        body += 0.35f * fastSin (v.plPhase * 2.01) * fm;
        float click = noise() * v.plClick * clamp01 (params.pluckSharp);
        float pl = body * 0.7f + click * 0.9f;
        pl = v.plLp.process (pl);
        pl = softClip (pl * (1.0f + clamp01 (params.pluckSharp) * 1.5f));

        pl *= v.plEnv * v.vel * 0.50f;
        // Also scale with amp sustain presence for held notes
        pl *= (0.55f + 0.45f * std::max (amp, 0.15f));

        const float g = dbToGain (params.pluckGainDb);
        outL += pl * g * 1.02f;
        outR += pl * g * 0.98f;
    }
    else
    {
        v.plEnv = 0;
    }

    // ========== Bell / ice stab ==========
    if (params.bellOn && v.bellEnv > 0.0001f)
    {
        const float dec = std::max (0.03f, params.bellDecay);
        v.bellEnv *= std::exp (-invSr * (3.5f / dec));
        const float ratio = 4.0f + clamp01 (params.bellTone) * 7.0f;
        v.bellMod += w * (double) ratio;
        v.bellPhase += w * 2.0 + (double) (fastSin (v.bellMod) * 3.2f);
        float bell = fastSin (v.bellPhase) * 0.55f + fastSin (v.bellMod) * 0.45f;
        bell += 0.2f * fastSin (v.bellPhase * 5.04);
        bell *= v.bellEnv * clamp01 (params.bellMix) * v.vel * 0.42f;
        const float g = dbToGain (params.bellGainDb);
        outL += bell * g * 1.06f;
        outR += bell * g * 0.94f;
    }
    else
    {
        v.bellEnv = 0;
    }

    // Chaos timbre: tiny noise grit on voice
    if (chaos > 0.001f)
    {
        float grit = noise() * chaos * 0.04f * amp;
        outL += grit; outR += grit * 0.9f;
    }

    // Per-voice filter with resonance boost
    const float res = clamp01 (params.resonance);
    float mixed = 0.5f * (outL + outR);
    float f = v.voiceLp.process (mixed + mixed * res * 0.22f);
    // restore stereo tilt
    float fL = f + (outL - outR) * 0.25f;
    float fR = f - (outL - outR) * 0.25f;

    L += fL;
    R += fR;

    if (! ampAlive && v.plEnv < 0.001f && v.bellEnv < 0.001f)
        v.active = false;
}

void ChromeEngine::process (float* left, float* right, int numSamples) noexcept
{
    applyMacroTargets();

    float peak = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float L = 0, R = 0;
        for (auto& v : voices)
            if (v.active)
                renderVoice (v, L, R);

        // Edge: high-pass air blend
        const float edge = edgeSm.next();
        if (edge > 0.001f)
        {
            float aL = edgeHpL.process (L);
            float aR = edgeHpR.process (R);
            L += aL * edge * 0.50f;
            R += aR * edge * 0.50f;
        }

        // Heat: drive / saturation
        const float heat = heatSm.next();
        const float drive = 1.0f + heat * 3.2f;
        L = softClip (L * drive) * (0.90f / (0.55f + heat * 0.45f));
        R = softClip (R * drive) * (0.90f / (0.55f + heat * 0.45f));

        // Space (short plate/room)
        const float space = spaceSm.next();
        delayL[(size_t) delayPos] = L;
        delayR[(size_t) delayPos] = R;
        const int t1 = (delayPos + kDelayLen - (int) (0.018 * sr)) % kDelayLen;
        const int t2 = (delayPos + kDelayLen - (int) (0.033 * sr)) % kDelayLen;
        const int t3 = (delayPos + kDelayLen - (int) (0.052 * sr)) % kDelayLen;
        const int t4 = (delayPos + kDelayLen - (int) (0.079 * sr)) % kDelayLen;
        float wetL = delayL[(size_t) t1] * 0.42f + delayR[(size_t) t2] * 0.28f
                   + delayL[(size_t) t3] * 0.18f + delayR[(size_t) t4] * 0.12f;
        float wetR = delayR[(size_t) t1] * 0.42f + delayL[(size_t) t2] * 0.28f
                   + delayR[(size_t) t3] * 0.18f + delayL[(size_t) t4] * 0.12f;
        L = lerp (L, L + wetL * 0.80f, space);
        R = lerp (R, R + wetR * 0.80f, space);
        delayPos = (delayPos + 1) % kDelayLen;

        // Width
        const float width = widthSm.next();
        float mid = 0.5f * (L + R);
        float side = 0.5f * (L - R) * (1.0f + width * 2.0f);
        L = mid + side;
        R = mid - side;

        chaosSm.next();
        cutSm.next();
        L = masterLpL.process (L);
        R = masterLpR.process (R);
        const float mg = masterSm.next();
        L *= mg; R *= mg;

        left[i] = L;
        right[i] = R;
        peak = std::max (peak, std::max (std::abs (L), std::abs (R)));
    }
    meter = peak;
}

} // namespace chaz
