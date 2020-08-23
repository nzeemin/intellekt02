
// SoundGen.h
//

#pragma once

#define SOUNDSAMPLERATE  22050

#define BUFSIZE	((SOUNDSAMPLERATE / 25) * 4)
#define BLOCK_COUNT	8
#define BLOCK_SIZE  BUFSIZE

void SoundGen_Initialize(WORD volume);
void SoundGen_Finalize();
void SoundGen_SetVolume(WORD volume);
void CALLBACK SoundGen_FeedDAC(unsigned short L, unsigned short R);
