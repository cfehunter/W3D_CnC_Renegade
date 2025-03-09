#pragma once

#ifndef _BINK_HO_STUB_INCLUDED_
#define _BINK_HO_STUB_INCLUDED_

// Copied from omniblade's bink stub, but implementations are in the header so we can ignore the x64 export issue

#define BINKSURFACE24 1
#define BINKSURFACE32 3
#define BINKSURFACE555 9
#define BINKSURFACE565 10
#define BINKPRELOADALL 0x00002000
#define BINKCOPYNOSCALING 0x70000000

typedef struct BINK
{
    unsigned int Width;
    unsigned int Height;
    unsigned int Frames;
    unsigned int FrameNum;
    unsigned int LastFrameNum;
    unsigned int FrameRate;
    unsigned int FrameRateDiv;
    /* Original struct has more members, but we only need these to match the ABI*/
} BINK, *HBINK;

typedef void *(*SndOpenCallback)(unsigned long param);

HBINK BinkOpen(const char* name, unsigned int flags) { return nullptr; }
void BinkSetSoundTrack(unsigned int total_tracks, unsigned int *tracks) { }
int BinkSetSoundSystem(SndOpenCallback open, unsigned long param) { return 0; }
void* BinkOpenDirectSound(unsigned long param) { return nullptr; }
void BinkClose(HBINK handle) {}
int BinkWait(HBINK handle) { return 0; }
int BinkDoFrame(HBINK handle) { return 0; }
int BinkCopyToBuffer(HBINK handle, void *dest, int destpitch, unsigned int destheight, unsigned int destx, unsigned int desty, unsigned int flags) { return 0; }
void BinkSetVolume(HBINK handle, unsigned int trackid, int volume) {}
void BinkNextFrame(HBINK handle) {}
void BinkGoto(HBINK handle, unsigned int frame, int flags) {}

#define BinkSoundUseDirectSound(x) BinkSetSoundSystem(BinkOpenDirectSound, (unsigned long)x)

#endif // _BINK_HO_STUB_INCLUDED_