#include "sound.h"
#include "../io/io.h"
#include "../timers/timer.h"
#include "../utility/utility.h"

// Simple delay function using busy waiting
void speaker_delay(uint32_t ms) {
    // Simple busy wait - adjust multiplier based on your CPU speed
    volatile uint32_t count = ms * 10000; // Adjust this value as needed
    while (count--) {
        // Just wait
    }
}

// Initialize the PC speaker
void speaker_init(void) {
    // Configure PIT channel 2 for square wave generation
    port_byte_out(PIT_COMMAND_PORT, 0xB6);
    speaker_disable();
}

// Enable the PC speaker
void speaker_enable(void) {
    uint8_t speaker_status = port_byte_in(SPEAKER_PORT);
    port_byte_out(SPEAKER_PORT, speaker_status | 0x03);
}

// Disable the PC speaker
void speaker_disable(void) {
    uint8_t speaker_status = port_byte_in(SPEAKER_PORT);
    port_byte_out(SPEAKER_PORT, speaker_status & 0xFC);
}

// Set the frequency of the PC speaker
void speaker_set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        speaker_disable();
        return;
    }
    
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Send the divisor to PIT channel 2
    port_byte_out(PIT_CHANNEL_2_PORT, (uint8_t)(divisor & 0xFF));
    port_byte_out(PIT_CHANNEL_2_PORT, (uint8_t)((divisor >> 8) & 0xFF));
}

// Play a tone for a specific duration
void speaker_play_tone(uint32_t frequency, uint32_t duration_ms) {
    if (frequency == 0) {
        // Rest/silence
        speaker_disable();
        speaker_delay(duration_ms);
        return;
    }
    
    speaker_set_frequency(frequency);
    speaker_enable();
    speaker_delay(duration_ms);
    speaker_disable();
}

// Simple beep sound
void speaker_beep(void) {
    speaker_play_tone(NOTE_A4, 200); // Shorter duration
}

// Test function to verify speaker works
void speaker_test(void) {
    print("Testing speaker...\n");
    
    // Simple ascending tones
    speaker_play_tone(440, 200);  // A4
    speaker_delay(50);
    speaker_play_tone(523, 200);  // C5
    speaker_delay(50);
    speaker_play_tone(659, 200);  // E5
    
    print("Speaker test complete\n");
}

// Play a melody from arrays of frequencies and durations
void speaker_play_melody(uint32_t* frequencies, uint32_t* durations, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        speaker_play_tone(frequencies[i], durations[i]);
        speaker_delay(20); // Small pause between notes
    }
}

// Play startup sound (simplified)
void speaker_play_startup_sound(void) {
    speaker_play_tone(NOTE_C4, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_E4, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_G4, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_C5, 300);
}

// Play error sound
void speaker_play_error_sound(void) {
    speaker_play_tone(NOTE_C5, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_A4, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_F4, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_C4, 300);
}

// Play success sound
void speaker_play_success_sound(void) {
    uint32_t notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
    for (int i = 0; i < 8; i++) {
        speaker_play_tone(notes[i], 100);
        speaker_delay(10);
    }
}

// Play notification sound
void speaker_play_notification_sound(void) {
    speaker_play_tone(NOTE_G4, 100);
    speaker_delay(50);
    speaker_play_tone(NOTE_C5, 100);
}

// Play a musical scale
void speaker_play_scale(void) {
    uint32_t scale[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
    for (int i = 0; i < 8; i++) {
        speaker_play_tone(scale[i], 200);
        speaker_delay(50);
    }
}

// Simplified Mario theme (just first few notes)
void speaker_play_mario_theme(void) {
    speaker_play_tone(NOTE_E5, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_E5, 150);
    speaker_delay(150);
    speaker_play_tone(NOTE_E5, 150);
    speaker_delay(150);
    speaker_play_tone(NOTE_C5, 150);
    speaker_delay(20);
    speaker_play_tone(NOTE_E5, 150);
    speaker_delay(150);
    speaker_play_tone(NOTE_G5, 300);
    speaker_delay(300);
    speaker_play_tone(NOTE_G4, 300);
}

// Frequency sweep effect (simplified)
void speaker_sweep(uint32_t start_freq, uint32_t end_freq, uint32_t duration_ms) {
    uint32_t steps = 20; // Fewer steps to avoid hanging
    uint32_t step_duration = duration_ms / steps;
    int32_t freq_step = (int32_t)(end_freq - start_freq) / (int32_t)steps;
    
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t current_freq = start_freq + (freq_step * i);
        speaker_set_frequency(current_freq);
        speaker_enable();
        speaker_delay(step_duration);
    }
    speaker_disable();
}
