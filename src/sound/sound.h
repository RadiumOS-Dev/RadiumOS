#ifndef PLAYSOUND_H
#define PLAYSOUND_H

#include <stdint.h>
#include <stdbool.h>

// PC Speaker frequency constants
#define PIT_FREQUENCY 1193180
#define SPEAKER_PORT 0x61
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL_2_PORT 0x42

// Musical note frequencies (in Hz)
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047

// Duration constants (in milliseconds)
#define DURATION_WHOLE    2000
#define DURATION_HALF     1000
#define DURATION_QUARTER  500
#define DURATION_EIGHTH   250
#define DURATION_SIXTEENTH 125

// Function prototypes
void speaker_init(void);
void speaker_enable(void);
void speaker_disable(void);
void speaker_set_frequency(uint32_t frequency);
void speaker_play_tone(uint32_t frequency, uint32_t duration_ms);
void speaker_beep(void);
void speaker_play_melody(uint32_t* frequencies, uint32_t* durations, uint32_t count);
void speaker_play_startup_sound(void);
void speaker_play_error_sound(void);
void speaker_play_success_sound(void);
void speaker_play_notification_sound(void);

// Musical scale functions
void speaker_play_scale(void);
void speaker_play_chord(uint32_t* frequencies, uint32_t count, uint32_t duration_ms);

// Advanced functions
void speaker_sweep(uint32_t start_freq, uint32_t end_freq, uint32_t duration_ms);
void speaker_play_mario_theme(void);
void speaker_play_tetris_theme(void);

#endif // PLAYSOUND_H
