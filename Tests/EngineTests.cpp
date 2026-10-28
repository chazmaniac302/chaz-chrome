#include "ChromeEngine.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++fails; } } while (0)

int main (int argc, char** argv)
{
    const char* outDir = argc > 1 ? argv[1] : ".";
    chaz::ChromeEngine eng;
    eng.prepare (48000.0, 512);

    chaz::ChromeParams p;
    p.whistleOn = true;
    p.fluteOn = true;
    p.pluckOn = true;
    p.bellOn = true;
    p.monoLegato = false;
    p.heat = 0.4f;
    p.space = 0.3f;
    p.width = 0.45f;
    p.edge = 0.55f;
    p.chaos = 0.1f;
    eng.setParams (p);

    // Minor pentatonic-ish top line: A4 C5 D5 E5 G5
    eng.noteOn (69, 0.85f); // A4
    const int N = 48000;
    std::vector<float> L ((size_t) N), R ((size_t) N);
    for (int off = 0; off < N; off += 256)
    {
        int n = std::min (256, N - off);
        eng.process (L.data() + off, R.data() + off, n);
        if (off == 6000)  { eng.noteOff (69); eng.noteOn (72, 0.82f); } // C5
        if (off == 12000) { eng.noteOff (72); eng.noteOn (74, 0.80f); } // D5
        if (off == 18000) { eng.noteOff (74); eng.noteOn (76, 0.85f); } // E5
        if (off == 24000) { eng.noteOff (76); eng.noteOn (79, 0.88f); } // G5
        if (off == 30000) { eng.noteOff (79); eng.noteOn (81, 0.80f); } // A5
        if (off == 36000) { eng.allNotesOff(); }
    }

    double peak = 0, energy = 0;
    for (int i = 0; i < N; ++i)
    {
        peak = std::max (peak, (double) std::abs (L[(size_t) i]));
        peak = std::max (peak, (double) std::abs (R[(size_t) i]));
        energy += (double) L[(size_t) i] * L[(size_t) i] + (double) R[(size_t) i] * R[(size_t) i];
    }
    std::printf ("peak=%.4f energy=%.2f meter=%.4f voices_end=%d\n",
                 peak, energy, eng.getMeter(), eng.getActiveVoices());
    CHECK (peak > 0.01);
    CHECK (peak < 8.0);
    CHECK (energy > 1.0);

    // Polyphony stress
    eng.reset();
    eng.setParams (p);
    for (int n = 48; n < 48 + 16; ++n)
        eng.noteOn (n, 0.7f);
    CHECK (eng.getActiveVoices() <= chaz::ChromeEngine::kMaxVoices);
    CHECK (eng.getActiveVoices() >= 8);

    // Mono legato / portamento path
    eng.reset();
    p.monoLegato = true;
    p.portaTime = 0.2f;
    eng.setParams (p);
    eng.noteOn (60, 0.9f);
    for (int i = 0; i < 1024; ++i) { float a=0,b=0; eng.process (&a, &b, 1); }
    eng.noteOn (67, 0.9f);
    CHECK (eng.getActiveVoices() <= 2);
    for (int i = 0; i < 2048; ++i) { float a=0,b=0; eng.process (&a, &b, 1); }
    eng.noteOff (67);
    eng.noteOff (60);

    // Pitch bend
    eng.reset();
    p.monoLegato = false;
    eng.setParams (p);
    eng.noteOn (72, 0.8f);
    eng.setPitchBend (1.0f);
    for (int i = 0; i < 512; ++i) { float a=0,b=0; eng.process (&a, &b, 1); }

    std::string path = std::string (outDir) + "/chaz-chrome-engine.raw";
#ifdef _WIN32
    _mkdir (outDir);
#else
    std::string mk = std::string ("mkdir -p \"") + outDir + "\"";
    std::system (mk.c_str());
#endif
    if (FILE* f = std::fopen (path.c_str(), "wb"))
    {
        std::fwrite (L.data(), sizeof (float), (size_t) N, f);
        std::fclose (f);
        std::printf ("wrote %s\n", path.c_str());
    }

    if (fails) { std::printf ("%d FAIL(s)\n", fails); return 1; }
    std::printf ("EngineTests OK\n");
    return 0;
}
