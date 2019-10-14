/******************************************************************************
/ padreMidi.h
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos (S&M), P. Bourdon
/
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#pragma once

// MIDI Commands Codes

#define MIDI_CMD_NONE						0x00
#define MIDI_CMD_NOTE_OFF					0x80 // note off
#define MIDI_CMD_NOTE_ON					0x90 // note on
#define MIDI_CMD_NOTE_PRESSURE				0xA0 // key pressure AKA Polyphonic Aftertouch
#define MIDI_CMD_CONTROL_CHANGE				0xB0 // control change
#define MIDI_CMD_PROGRAM_CHANGE				0xC0 // program change
#define MIDI_CMD_CHANNEL_PRESSURE			0xD0 // channel pressure AKA Aftertouch
#define MIDI_CMD_PITCHBEND					0xE0 // pitch bender

#define MIDI_CMD_COMMON_SYSEX				0xF0 // sysex (system exclusive) begin
#define MIDI_CMD_COMMON_MTC_QUARTER			0xF1 // MTC quarter frame
#define MIDI_CMD_COMMON_SONG_POS			0xF2 // song position
#define MIDI_CMD_COMMON_SONG_SELECT			0xF3 // song select
#define MIDI_CMD_COMMON_F4					0xF4 // UNDEFINED IN MIDI SPEC
#define MIDI_CMD_COMMON_F5					0xF5 // UNDEFINED IN MIDI SPEC
#define MIDI_CMD_COMMON_TUNE_REQUEST		0xF6 // tune request
#define MIDI_CMD_COMMON_SYSEX_END			0xF7 // end of sysex (EOX)
#define MIDI_CMD_COMMON_CLOCK				0xF8 // clock
#define MIDI_CMD_COMMON_START				0xFA // start
#define MIDI_CMD_COMMON_CONTINUE			0xFB // continue
#define MIDI_CMD_COMMON_STOP				0xFC // stop
#define MIDI_CMD_COMMON_SENSING				0xFE // active sensing
#define MIDI_CMD_COMMON_RESET				0xFF // reset

// MIDI Controllers Numbers
// Note: Controller numbers 120-127 are reserved for Channel Mode Messages, which rather than controlling
// sound parameters, affect the channel's operating mode

#define MIDI_CC000_MSB_BANK					0x00 // Bank Select
#define MIDI_CC001_MSB_MODWHEEL				0x01 // Modulation Wheel
#define MIDI_CC002_MSB_BREATH				0x02 // Breath Controller
#define MIDI_CC003_MSB_UNDEFINED			0x03 // Undefined
#define MIDI_CC004_MSB_FOOT					0x04 // Foot Controller
#define MIDI_CC005_MSB_PORTAMENTO_TIME		0x05 // Portamento time
#define MIDI_CC006_MSB_DATA_ENTRY			0x06 // Data Entry MSB
#define MIDI_CC007_MSB_CHANNEL_VOLUME		0x07 // Channel volume (formely Main volume)
#define MIDI_CC008_MSB_BALANCE				0x08 // Balance
#define MIDI_CC009_MSB_UNDEFINED			0x09 // Undefined
#define MIDI_CC010_MSB_PAN					0x0A // Pan
#define MIDI_CC011_MSB_EXPRESSION			0x0B // Expression Controller
#define MIDI_CC012_MSB_EFFECT1				0x0C // Effect Control 1
#define MIDI_CC013_MSB_EFFECT2				0x0D // Effect Control 2
#define MIDI_CC014_MSB_UNDEFINED			0x0E // Undefined
#define MIDI_CC015_MSB_UNDEFINED			0x0F // Undefined
#define MIDI_CC016_MSB_GENERAL_PURPOSE1		0x10 // General Purpose Controller 1
#define MIDI_CC017_MSB_GENERAL_PURPOSE2		0x11 // General Purpose Controller 2
#define MIDI_CC018_MSB_GENERAL_PURPOSE3		0x12 // General Purpose Controller 3
#define MIDI_CC019_MSB_GENERAL_PURPOSE4		0x13 // General Purpose Controller 4
#define MIDI_CC020_MSB_UNDEFINED			0x14 // Undefined
#define MIDI_CC021_MSB_UNDEFINED			0x15 // Undefined
#define MIDI_CC022_MSB_UNDEFINED			0x16 // Undefined
#define MIDI_CC023_MSB_UNDEFINED			0x17 // Undefined
#define MIDI_CC024_MSB_UNDEFINED			0x18 // Undefined
#define MIDI_CC025_MSB_UNDEFINED			0x19 // Undefined
#define MIDI_CC026_MSB_UNDEFINED			0x1A // Undefined
#define MIDI_CC027_MSB_UNDEFINED			0x1B // Undefined
#define MIDI_CC028_MSB_UNDEFINED			0x1C // Undefined
#define MIDI_CC029_MSB_UNDEFINED			0x1D // Undefined
#define MIDI_CC030_MSB_UNDEFINED			0x1E // Undefined
#define MIDI_CC031_MSB_UNDEFINED			0x1F // Undefined
#define MIDI_CC032_LSB_BANK					0x20 // LSB for Control 0 (Bank Select)
#define MIDI_CC033_LSB_MODWHEEL				0x21 // LSB for Control 1 (Modulation Wheel or Lever)
#define MIDI_CC034_LSB_BREATH				0x22 // LSB for Control 2 (Breath Controller)
#define MIDI_CC035_LSB_UNDEFINED			0x23 // LSB for Control 3 (Undefined)
#define MIDI_CC036_LSB_FOOT					0x24 // LSB for Control 4 (Foot Controller)
#define MIDI_CC037_LSB_PORTAMENTO_TIME		0x25 // LSB for Control 5 (Portamento Time)
#define MIDI_CC038_LSB_DATA_ENTRY			0x26 // LSB for Control 6 (Data Entry)
#define MIDI_CC039_LSB_MAIN_VOLUME			0x27 // LSB for Control 7 (Channel Volume, formerly Main Volume)
#define MIDI_CC040_LSB_BALANCE				0x28 // LSB for Control 8 (Balance)
#define MIDI_CC041_LSB_UNDEFINED			0x29 // LSB for Control 9 (Undefined)
#define MIDI_CC042_LSB_PAN					0x2A // LSB for Control 10 (Pan)
#define MIDI_CC043_LSB_EXPRESSION			0x2B // LSB for Control 11 (Expression Controller)
#define MIDI_CC044_LSB_EFFECT1				0x2C // LSB for Control 12 (Effect control 1)
#define MIDI_CC045_LSB_EFFECT2				0x2D // LSB for Control 13 (Effect control 2)
#define MIDI_CC046_LSB_UNDEFINED			0x2E // LSB for Control 14 (Undefined)
#define MIDI_CC047_LSB_UNDEFINED			0x2F // LSB for Control 15 (Undefined)
#define MIDI_CC048_LSB_GENERAL_PURPOSE1		0x30 // LSB for Control 16 (General Purpose Controller 1)
#define MIDI_CC049_LSB_GENERAL_PURPOSE2		0x31 // LSB for Control 17 (General Purpose Controller 2)
#define MIDI_CC050_LSB_GENERAL_PURPOSE3		0x32 // LSB for Control 18 (General Purpose Controller 3)
#define MIDI_CC051_LSB_GENERAL_PURPOSE4		0x33 // LSB for Control 19 (General Purpose Controller 4)
#define MIDI_CC052_LSB_UNDEFINED			0x34 // LSB for Control 20 (Undefined)
#define MIDI_CC053_LSB_UNDEFINED			0x35 // LSB for Control 21 (Undefined)
#define MIDI_CC054_LSB_UNDEFINED			0x36 // LSB for Control 22 (Undefined)
#define MIDI_CC055_LSB_UNDEFINED			0x37 // LSB for Control 23 (Undefined)
#define MIDI_CC056_LSB_UNDEFINED			0x38 // LSB for Control 24 (Undefined)
#define MIDI_CC057_LSB_UNDEFINED			0x39 // LSB for Control 25 (Undefined)
#define MIDI_CC058_LSB_UNDEFINED			0x3A // LSB for Control 26 (Undefined)
#define MIDI_CC059_LSB_UNDEFINED			0x3B // LSB for Control 27 (Undefined)
#define MIDI_CC060_LSB_UNDEFINED			0x3C // LSB for Control 28 (Undefined)
#define MIDI_CC061_LSB_UNDEFINED			0x3D // LSB for Control 29 (Undefined)
#define MIDI_CC062_LSB_UNDEFINED			0x3E // LSB for Control 30 (Undefined)
#define MIDI_CC063_LSB_UNDEFINED			0x3F // LSB for Control 31 (Undefined)
#define MIDI_CC064_SUSTAIN					0x40 // Damper Pedal on/off (Sustain) 	<63 off, >64 on
#define MIDI_CC065_PORTAMENTO				0x41 // Portamento On/Off 	<63 off, >64 on
#define MIDI_CC066_SOSTENUTO				0x42 // Sostenuto On/Off 	<63 off, >64 on
#define MIDI_CC067_SOFT_PEDAL				0x43 // Soft Pedal On/Off 	<63 off, >64 on
#define MIDI_CC068_LEGATO_FOOTSWITCH		0x44 // Legato Footswitch 	<63 Normal, >64 Legato
#define MIDI_CC069_HOLD2					0x45 // Hold 2 	<63 off, >64 on
#define MIDI_CC070_LSB_SC1_SOUND_VARIATION	0x46 // Sound Controller 1 (default: Sound Variation)
#define MIDI_CC071_LSB_SC2_TIMBRE			0x47 // Sound Controller 2 (default: Timbre/Harmonic Intens.)
#define MIDI_CC072_LSB_SC3_RELEASE_TIME		0x48 // Sound Controller 3 (default: Release Time)
#define MIDI_CC073_LSB_SC4_ATTACK_TIME		0x49 // Sound Controller 4 (default: Attack Time)
#define MIDI_CC074_LSB_SC5_BRIGHTNESS		0x4A // Sound Controller 5 (default: Brightness)
#define MIDI_CC075_LSB_SC6					0x4B // Sound Controller 6 (default: Decay Time - see MMA RP-021)
#define MIDI_CC076_LSB_SC7					0x4C // Sound Controller 7 (default: Vibrato Rate - see MMA RP-021)
#define MIDI_CC077_LSB_SC8					0x4D // Sound Controller 8 (default: Vibrato Depth - see MMA RP-021)
#define MIDI_CC078_LSB_SC9					0x4E // Sound Controller 9 (default: Vibrato Delay - see MMA RP-021)
#define MIDI_CC079_LSB_SC10					0x4F // Sound Controller 10 (default undefined - see MMA RP-021)
#define MIDI_CC080_LSB_GENERAL_PURPOSE5		0x50 // General Purpose Controller 5
#define MIDI_CC081_LSB_GENERAL_PURPOSE6		0x51 // General Purpose Controller 6
#define MIDI_CC082_LSB_GENERAL_PURPOSE7		0x52 // General Purpose Controller 7
#define MIDI_CC083_LSB_GENERAL_PURPOSE8		0x53 // General Purpose Controller 8
#define MIDI_CC084_LSB_PORTAMENTO_CONTROL	0x54 // Portamento control
#define MIDI_CC085_UNDEFINED				0x55 // Undefined
#define MIDI_CC086_UNDEFINED				0x56 // Undefined
#define MIDI_CC087_UNDEFINED				0x57 // Undefined
#define MIDI_CC088_UNDEFINED				0x58 // Undefined
#define MIDI_CC089_UNDEFINED				0x59 // Undefined
#define MIDI_CC090_UNDEFINED				0x5A // Undefined
#define MIDI_CC091_LSB_E1_REVERB_DEPTH		0x5B // Effects 1 Depth (default: Reverb Send Level - see MMA RP-023) (formerly External Effects Depth)
#define MIDI_CC092_LSB_E2_TREMOLO_DEPTH		0x5C // Effects 2 Depth (formerly Tremolo Depth)
#define MIDI_CC093_LSB_E3_CHORUS_DEPTH		0x5D // Effects 3 Depth (default: Chorus Send Level - see MMA RP-023)
#define MIDI_CC094_LSB_E4_DETUNE_DEPTH		0x5E // Effects 4 Depth (formerly Celeste [Detune] Depth)
#define MIDI_CC095_LSB_E5_PHASER_DEPTH		0x5F // Effects 5 Depth (formerly Phaser Depth)
#define MIDI_CC096_DATA_INCREMENT			0x60 // Data Increment (Data Entry +1) (see MMA RP-018)
#define MIDI_CC097_DATA_DECREMENT			0x61 // Data Decrement (Data Entry -1) (see MMA RP-018)
#define MIDI_CC098_NONREG_PARM_NUM_LSB		0x62 // Non-Registered Parameter Number (NRPN) - LSB
#define MIDI_CC099_NONREG_PARM_NUM_MSB		0x63 // Non-Registered Parameter Number (NRPN) - MSB
#define MIDI_CC100_REGIST_PARM_NUM_LSB		0x64 // Registered Parameter Number (RPN) - LSB*
#define MIDI_CC101_REGIST_PARM_NUM_MSB		0x65 // Registered Parameter Number (RPN) - MSB*
#define MIDI_CC102_UNDEFINED				0x66 // Undefined
#define MIDI_CC103_UNDEFINED				0x67 // Undefined
#define MIDI_CC104_UNDEFINED				0x68 // Undefined
#define MIDI_CC105_UNDEFINED				0x69 // Undefined
#define MIDI_CC106_UNDEFINED				0x6A // Undefined
#define MIDI_CC107_UNDEFINED				0x6B // Undefined
#define MIDI_CC108_UNDEFINED				0x6C // Undefined
#define MIDI_CC109_UNDEFINED				0x6D // Undefined
#define MIDI_CC110_UNDEFINED				0x6E // Undefined
#define MIDI_CC111_UNDEFINED				0x6F // Undefined
#define MIDI_CC112_UNDEFINED				0x70 // Undefined
#define MIDI_CC113_UNDEFINED				0x71 // Undefined
#define MIDI_CC114_UNDEFINED				0x72 // Undefined
#define MIDI_CC115_UNDEFINED				0x73 // Undefined
#define MIDI_CC116_UNDEFINED				0x74 // Undefined
#define MIDI_CC117_UNDEFINED				0x75 // Undefined
#define MIDI_CC118_UNDEFINED				0x76 // Undefined
#define MIDI_CC119_UNDEFINED				0x77 // Undefined
#define MIDI_CC120_ALL_SOUNDS_OFF			0x78 // [Channel Mode Message] All Sound Off: 0
#define MIDI_CC121_RESET_CONTROLLERS		0x79 // [Channel Mode Message] Reset All Controllers: 0
#define MIDI_CC122_LOCAL_CONTROL_SWITCH		0x7A // [Channel Mode Message] Local Control On/Off: 0... 127 (off, on)
#define MIDI_CC123_ALL_NOTES_OFF			0x7B // [Channel Mode Message] All Notes Off: 0
#define MIDI_CC124_OMNI_OFF					0x7C // [Channel Mode Message] Omni Mode Off (+ all notes off): 0
#define MIDI_CC125_OMNI_ON					0x7D // [Channel Mode Message] Omni Mode On (+ all notes off): 0
#define MIDI_CC126_MONO1					0x7E // [Channel Mode Message] Poly Mode On/Off (+ all notes off) **
// ** Note: This equals the number of channels, or zero if the number of channels equals the number of voices in the receiver
#define MIDI_CC127_MONO2					0x7F // [Channel Mode Message] Poly Mode On (+ mono off +all notes off): 0

// Registered Parameter Numbers
// To set or change the value of a Registered Parameter:
//
// 1. Send two Control Change messages using Control Numbers 101 (65H) and 100 (64H) to select the desired
//    Registered Parameter Number, as per the following table.
//
// 2. To set the selected Registered Parameter to a specific value, send a Control Change messages to the
//    Data Entry MSB controller (Control Number 6). If the selected Registered Parameter requires the LSB
//    to be set, send another Control Change message to the Data Entry LSB controller (Control Number 38).
//
// 3. To make a relative adjustment to the selected Registered Parameter's current value, use the Data
//    Increment or Data Decrement controllers (Control Numbers 96 and 97).
//
// Pitch Bend Sensitivity:	Control 101 (MSB) = 0, Control 100 (LSB) = 0
// 				Data Entry Value: MSB = +/- semitones, LSB =+/--cents
//
// Channel Fine Tuning:		Control 101 (MSB) = 0, Control 100 (LSB) = 1
// 				Data Entry Value: Resolution 100/8192 cents. 00H 00H = -100, cents 40H 00H = A440, 7FH 7FH = +100 cents
//
// Channel Coarse Tuning:	Control 101 (MSB) = 0, Control 100 (LSB) = 2
// 				Data Entry Value: Only MSB used, Resolution 100 cents. 00H = -6400 cents, 40H = A440, 7FH = +6300 cents
//
// Tuning Program Change:	Control 101 (MSB) = 0, Control 100 (LSB) = 3
// 				Data Entry Value: Tuning Program Number
//
// Tuning Bank Select:		Control 101 (MSB) = 0, Control 100 (LSB) = 4
// 				Data Entry Value: Tuning Bank Number
//
// Modulation Depth Range:	Control 101 (MSB) = 0, Control 100 (LSB) = 5
// 				Data Entry Value: For GM2, defined in GM2 Specification. For other systems, defined by manufacturer

// Waldorf MicroQ MIDI Controllers Numbers

#define MIDI_WALDORFmQ_CC000_NOT_USED				0x00 // Not Used
#define MIDI_WALDORFmQ_CC001_MODWHEEL				0x01 // Modulation Wheel*
#define MIDI_WALDORFmQ_CC002_MSB_BREATH				0x02 // Breath Controller*
#define MIDI_WALDORFmQ_CC003_NOT_USED				0x03 // Not Used
#define MIDI_WALDORFmQ_CC004_FOOT					0x04 // Foot Controller*
#define MIDI_WALDORFmQ_CC005_GLIDE_RATE				0x05 // Glide Rate
#define MIDI_WALDORFmQ_CC006_NOT_USED				0x06 // Not Used
#define MIDI_WALDORFmQ_CC007_CHANNEL_VOLUME			0x07 // Channel volume*
#define MIDI_WALDORFmQ_CC008_NOT_USED				0x08 // Not Used
#define MIDI_WALDORFmQ_CC009_NOT_USED				0x09 // Not Used
#define MIDI_WALDORFmQ_CC010_PAN					0x0A // Pan (= L64... R63)
#define MIDI_WALDORFmQ_CC011_NOT_USED				0x0B // Not Used
#define MIDI_WALDORFmQ_CC012_ARP_RANGE				0x0C // Arp Range: 0... 9 (1... 10 octaves)
#define MIDI_WALDORFmQ_CC013_ARP_LENGTH				0x0D // Arp Length: 0... 15 (1... 16 steps)
#define MIDI_WALDORFmQ_CC014_ARP_ACTIVE				0x0E // Arp Active: 0... 3 (off, on, one shot, hold)
#define MIDI_WALDORFmQ_CC015_LFO1_SHAPE				0x0F // LFO1 Shape: 0... 5 (sine, triangle, square, sawtooth, random, S&H)
#define MIDI_WALDORFmQ_CC016_LFO1_SPEED				0x10 // LFO1 Speed
#define MIDI_WALDORFmQ_CC017_LFO1_SYNC				0x11 // LFO1 Sync: 0...1 (off, on)
#define MIDI_WALDORFmQ_CC018_LFO1_DELAY				0x12 // LFO1 Delay
#define MIDI_WALDORFmQ_CC019_LFO2_SHAPE				0x13 // LFO2 Shape: 0... 5 (sine, triangle, square, sawtooth, random, S&H)
#define MIDI_WALDORFmQ_CC020_LFO2_SPEED				0x14 // LFO2 Speed
#define MIDI_WALDORFmQ_CC021_LFO2_SYNC				0x15 // LFO2 Sync: 0...1 (off, on)
#define MIDI_WALDORFmQ_CC022_LFO2_DELAY				0x16 // LFO2 Delay
#define MIDI_WALDORFmQ_CC023_LFO3_SHAPE				0x17 // LFO3 Shape: 0... 5 (sine, triangle, square, sawtooth, random, S&H)
#define MIDI_WALDORFmQ_CC024_LFO3_SPEED				0x18 // LFO3 Speed
#define MIDI_WALDORFmQ_CC025_LFO3_SYNC				0x19 // LFO3 Sync: 0...1 (off, on)
#define MIDI_WALDORFmQ_CC026_LFO3_DELAY				0x1A // LFO3 Delay
#define MIDI_WALDORFmQ_CC027_OSC1_OCTAVE			0x1B // Osc 1 Octave: 16, 28, 40... 112 (128'... 1/2')
#define MIDI_WALDORFmQ_CC028_OSC1_SEMITONE			0x1C // Osc 1 Semitone: 52... 76 (-12... +12)
#define MIDI_WALDORFmQ_CC029_OSC1_DETUNE			0x1D // Osc 1 Detune (-64... +63)
#define MIDI_WALDORFmQ_CC030_OSC1_FM				0x1E // Osc 1 FM
#define MIDI_WALDORFmQ_CC031_OSC1_SHAPE				0x1F // Osc 1 Shape: 0... 5 (pulse, saw, triangle, sine, alt 1, alt 2)
#define MIDI_WALDORFmQ_CC032_BANK_SELECT			0x20 // Bank Select: 0... 3 (Bank A... D)
#define MIDI_WALDORFmQ_CC033_OSC1_PW				0x21 // Osc 1 PW
#define MIDI_WALDORFmQ_CC034_OSC1_PWM				0x22 // Osc 1 PWM (-64... +63)
#define MIDI_WALDORFmQ_CC035_OSC2_OCTAVE			0x23 // Osc 2 Octave: 16, 28, 40... 112 (128'... 1/2')
#define MIDI_WALDORFmQ_CC036_OSC2_SEMITONE			0x24 // Osc 2 Semitone: 52... 76 (-12... +12)
#define MIDI_WALDORFmQ_CC037_OSC2_DETUNE			0x25 // Osc 2 Detune (-64... +63)
#define MIDI_WALDORFmQ_CC038_OSC2_FM				0x26 // Osc 2 FM
#define MIDI_WALDORFmQ_CC039_OSC2_SHAPE				0x27 // Osc 2 Shape: 0... 5 (pulse, saw, triangle, sine, alt 1, alt 2)
#define MIDI_WALDORFmQ_CC040_OSC2_PW				0x28 // Osc 2 PW
#define MIDI_WALDORFmQ_CC041_OSC2_PWM				0x29 // Osc 2 PWM (-64... +63)
#define MIDI_WALDORFmQ_CC042_OSC3_OCTAVE			0x2A // Osc 3 Octave: 16, 28, 40... 112 (128'... 1/2')
#define MIDI_WALDORFmQ_CC043_OSC3_SEMITONE			0x2B // Osc 3 Semitone: 52... 76 (-12... +12)
#define MIDI_WALDORFmQ_CC044_OSC3_DETUNE			0x2C // Osc 3 Detune (-64... +63)
#define MIDI_WALDORFmQ_CC045_OSC3_FM				0x2D // Osc 3 FM
#define MIDI_WALDORFmQ_CC046_OSC3_SHAPE				0x2E // Osc 3 Shape: 0... 5 (pulse, saw, triangle, sine, alt 1, alt 2)
#define MIDI_WALDORFmQ_CC047_OSC3_PW				0x2F // Osc 3 PW
#define MIDI_WALDORFmQ_CC048_OSC3_PWM				0x30 // Osc 3 PWM (-64... +63)
#define MIDI_WALDORFmQ_CC049_SYNC					0x31 // Sync: 0... 1 (off, on)
#define MIDI_WALDORFmQ_CC050_PITCHMOD				0x32 // Pitchmod (-64... +63)
#define MIDI_WALDORFmQ_CC051_GLIDE_MODE				0x33 // Glide Mode (0... 9)
#define MIDI_WALDORFmQ_CC052_OSC1_LEVEL				0x34 // Osc 1 Level
#define MIDI_WALDORFmQ_CC053_OSC1_BALANCE			0x35 // Osc 1 Balance
#define MIDI_WALDORFmQ_CC054_RINGMOD_LEVEL			0x36 // Ringmod Level
#define MIDI_WALDORFmQ_CC055_RINGMOD_BALANCE		0x37 // Ringmod Balance
#define MIDI_WALDORFmQ_CC056_OSC2_LEVEL				0x38 // Osc 2 Level
#define MIDI_WALDORFmQ_CC057_OSC2_BALANCE			0x35 // Osc 2 Balance
#define MIDI_WALDORFmQ_CC058_OSC3_LEVEL				0x3A // Osc 3 Level
#define MIDI_WALDORFmQ_CC059_OSC3_BALANCE			0x3B // Osc 3 Balance
#define MIDI_WALDORFmQ_CC060_NE_LEVEL				0x3C // N/E Level
#define MIDI_WALDORFmQ_CC061_NE_BALANCE				0x3D // N/E Balance
#define MIDI_WALDORFmQ_CC062_NOT_USED				0x3E // Not Used
#define MIDI_WALDORFmQ_CC063_NOT_USED				0x3F // Not Used
#define MIDI_WALDORFmQ_CC064_SUSTAIN				0x40 // Sustain Pedal: <63 off, >64 on
#define MIDI_WALDORFmQ_CC065_GLIDE_ACTIVE			0x41 // Glide Active: <63 off, >64 on
#define MIDI_WALDORFmQ_CC066_SOSTENUTO				0x42 // Sostenuto Off/On 	<63 off, >64 on
#define MIDI_WALDORFmQ_CC067_ROUTING				0x43 // Routing Serial/Parallel <63 Serial, >64 Parallel
#define MIDI_WALDORFmQ_CC068_FILTER1_TYPE			0x44 // Filter 1 Type: 0... 10 (Bypass, 24dB LP, 12dB LP, 24dB BP, 12dB BP, 24dB HP, 12dB HP, 24dB Notch, 12dB Notch, Comb+, Comb-)
#define MIDI_WALDORFmQ_CC069_FILTER1_CUTOFF			0x45 // Filter 1 Cutoff
#define MIDI_WALDORFmQ_CC070_FILTER1_RESONANCE		0x46 // Filter 1 Resonance
#define MIDI_WALDORFmQ_CC071_FILTER1_DRIVE			0x47 // Filter 1 Drive
#define MIDI_WALDORFmQ_CC072_FILTER1_KEYTRACK		0x48 // Filter 1 Keytrack (-200%... +197%)
#define MIDI_WALDORFmQ_CC073_FILTER1_ENV_AMOUNT		0x49 // Filter 1 Env. Amount (-64... 63)
#define MIDI_WALDORFmQ_CC074_FILTER1_ENV_VELOCITY	0x4A // Filter 1 Env. Velocity (-64... 63)
#define MIDI_WALDORFmQ_CC075_FILTER1_CUTOFFMOD		0x4B // Filter 1 Cutoff Mod (-64... 63)
#define MIDI_WALDORFmQ_CC076_FILTER1_FM				0x4C // Filter 1 FM (off, 1... 127)
#define MIDI_WALDORFmQ_CC077_FILTER1_PAN			0x4D // Filter 1 Pan (L64... Center... R63)
#define MIDI_WALDORFmQ_CC078_FILTER1_PANMOD			0x4E // Filter 1 Pan Mod (-64... 63)
#define MIDI_WALDORFmQ_CC079_FILTER2_TYPE			0x4F // Filter 2 Type: 0... 10 (Bypass, 24dB LP, 12dB LP, 24dB BP, 12dB BP, 24dB HP, 12dB HP, 24dB Notch, 12dB Notch, Comb+, Comb-)
#define MIDI_WALDORFmQ_CC080_FILTER2_CUTOFF			0x50 // Filter 2 Cutoff
#define MIDI_WALDORFmQ_CC081_FILTER2_RESONANCE		0x51 // Filter 2 Resonance
#define MIDI_WALDORFmQ_CC082_FILTER2_DRIVE			0x52 // Filter 2 Drive
#define MIDI_WALDORFmQ_CC083_FILTER2_KEYTRACK		0x53 // Filter 2 Keytrack (-200%... +197%)
#define MIDI_WALDORFmQ_CC084_FILTER2_ENV_AMOUNT		0x54 // Filter 2 Env. Amount (-64... 63)
#define MIDI_WALDORFmQ_CC085_FILTER2_ENV_VELOCITY	0x55 // Filter 2 Env. Velocity (-64... 63)
#define MIDI_WALDORFmQ_CC086_FILTER2_CUTOFFMOD		0x56 // Filter 2 Cutoff Mod (-64... 63)
#define MIDI_WALDORFmQ_CC087_FILTER2_FM				0x57 // Filter 2 FM (off, 1... 127)
#define MIDI_WALDORFmQ_CC088_FILTER2_PAN			0x58 // Filter 2 Pan (L64... Center... R63)
#define MIDI_WALDORFmQ_CC089_FILTER2_PANMOD			0x59 // Filter 2 Pan Mod (-64... 63)
#define MIDI_WALDORFmQ_CC090_AMP_VOLUME				0x5A // Amp Volume
#define MIDI_WALDORFmQ_CC091_AMP_VELOCITY			0x5B // Amp Velocity (-64... 63)
#define MIDI_WALDORFmQ_CC092_AMP_MOD				0x5C // Amp Mod (-64... 63)
#define MIDI_WALDORFmQ_CC093_FX1_MIX				0x5D // FX1 Mix
#define MIDI_WALDORFmQ_CC094_FX2_MIX				0x5E // FX2 Mix
#define MIDI_WALDORFmQ_CC095_FE_ATTACK				0x5F // Filter Enveloppe Attack
#define MIDI_WALDORFmQ_CC096_FE_DECAY				0x60 // Filter Enveloppe Decay
#define MIDI_WALDORFmQ_CC097_FE_SUSTAIN				0x61 // Filter Enveloppe Sustain
#define MIDI_WALDORFmQ_CC098_FE_DECAY2				0x62 // Filter Enveloppe Decay 2
#define MIDI_WALDORFmQ_CC099_FE_SUSTAIN2			0x63 // Filter Enveloppe Sustain 2
#define MIDI_WALDORFmQ_CC100_FE_RELEASE				0x64 // Filter Enveloppe Release
#define MIDI_WALDORFmQ_CC101_AE_ATTACK				0x65 // Amp Enveloppe Attack
#define MIDI_WALDORFmQ_CC102_AE_DECAY				0x66 // Amp Enveloppe Decay
#define MIDI_WALDORFmQ_CC103_AE_SUSTAIN				0x67 // Amp Enveloppe Sustain
#define MIDI_WALDORFmQ_CC104_AE_DECAY2				0x68 // Amp Enveloppe Decay 2
#define MIDI_WALDORFmQ_CC105_AE_SUSTAIN2			0x69 // Amp Enveloppe Sustain 2
#define MIDI_WALDORFmQ_CC106_AE_RELEASE				0x6A // Amp Enveloppe Release
#define MIDI_WALDORFmQ_CC107_E3_ATTACK				0x6B // Enveloppe 3 Attack
#define MIDI_WALDORFmQ_CC108_E3_DECAY				0x6C // Enveloppe 3 Decay
#define MIDI_WALDORFmQ_CC109_E3_SUSTAIN				0x6D // Enveloppe 3 Sustain
#define MIDI_WALDORFmQ_CC110_E3_DECAY2				0x6E // Enveloppe 3 Decay 2
#define MIDI_WALDORFmQ_CC111_E3_SUSTAIN2			0x69 // Enveloppe 3 Sustain 2
#define MIDI_WALDORFmQ_CC112_E3_RELEASE				0x70 // Enveloppe 3 Release
#define MIDI_WALDORFmQ_CC113_E4_ATTACK				0x71 // Enveloppe 4 Attack
#define MIDI_WALDORFmQ_CC114_E4_DECAY				0x72 // Enveloppe 4 Decay
#define MIDI_WALDORFmQ_CC115_E4_SUSTAIN				0x73 // Enveloppe 4 Sustain
#define MIDI_WALDORFmQ_CC116_E4_DECAY2				0x74 // Enveloppe 4 Decay 2
#define MIDI_WALDORFmQ_CC117_E4_SUSTAIN2			0x75 // Enveloppe 4 Sustain 2
#define MIDI_WALDORFmQ_CC118_E4_RELEASE				0x76 // Enveloppe 4 Release
#define MIDI_WALDORFmQ_CC119_NOT_USED				0x77 // Not Used
#define MIDI_WALDORFmQ_CC120_ALL_SOUNDS_OFF			0x78 // [Channel Mode Message] All Sound Off: 0
#define MIDI_WALDORFmQ_CC121_RESET_CONTROLLERS		0x79 // [Channel Mode Message] Reset All Controllers: 0
#define MIDI_WALDORFmQ_CC122_LOCAL_CONTROL_SWITCH	0x7A // [Channel Mode Message] Local Control Off/On: 0... 127
#define MIDI_WALDORFmQ_CC123_ALL_NOTES_OFF			0x7B // [Channel Mode Message] All Notes Off: 0
#define MIDI_WALDORFmQ_CC124_NOT_USED				0x7C // Not Used
#define MIDI_WALDORFmQ_CC125_NOT_USED				0x7D // Not Used
#define MIDI_WALDORFmQ_CC126_NOT_USED				0x7E // Not Used
#define MIDI_WALDORFmQ_CC127_NOT_USED				0x7F // Not Used

// Clavia Nord Modular G2 MIDI Controllers Numbers

#define MIDI_CLAVIAG2_CC000_BANKSELECT			0x00 // Bank Select
#define MIDI_CLAVIAG2_CC001_MODWHEEL			0x01 // Modulation Wheel
#define MIDI_CLAVIAG2_CC007_PATCH_VOLUME		0x07 // Patch volume
#define MIDI_CLAVIAG2_CC011_EXPRESSION			0x0B // Expression Pedal
#define MIDI_CLAVIAG2_CC017_OCTAVE_SHIFT		0x11 // Octave Shift
#define MIDI_CLAVIAG2_CC018_KEYBOARD_HOLD		0x12 // Keyboard Hold
#define MIDI_CLAVIAG2_CC019_MODE_SELECT			0x13 // Performance/Patch Mode Select. Preassigned for Global MIDI channel only
#define MIDI_CLAVIAG2_CC032_BANK_SELECT			0x20 // Bank Select
#define MIDI_CLAVIAG2_CC064_SUSTAIN				0x40 // Sustain Pedal: <63 off, >64 on
#define MIDI_CLAVIAG2_CC070_SOUND_VARIATION		0x46 // Sound Variation
#define MIDI_CLAVIAG2_CC080_MASTERCLOCK			0x50 // MASTERCLOCK/MIDI CLOCK Stop/Run. Preassigned for Global MIDI channel only
#define MIDI_CLAVIAG2_CC096_MORPHGROUP8			0x60 // G2X Global Modwheel 1 (Morph Group 8)
#define MIDI_CLAVIAG2_CC097_MORPHGROUP5			0x61 // G2X Global Modwheel 2 (Morph Group 5)
#define MIDI_CLAVIAG2_CC121_RESET_CONTROLLERS	0x79 // [Channel Mode Message] Reset All Controllers: 0
#define MIDI_CLAVIAG2_CC123_ALL_NOTES_OFF		0x7B // [Channel Mode Message] All Notes Off: 0

// Behringer DSP2024 MIDI Controllers Numbers

#define MIDI_BEHRINGER_DSP2024_CC000_BANKSELECT		0x00 // Bank Select: 0, 1 (ROM, RAM)
#define MIDI_BEHRINGER_DSP2024_CC102_ALGORITHM		0x66 // Algorithm Name: 0... 70
#define MIDI_BEHRINGER_DSP2024_CC103_EDITA			0x67 // Edit A (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC104_EDITB			0x68 // Edit B (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC105_EDITC			0x69 // Edit C (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC106_EDITD			0x6A // Edit D (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC107_EDITE			0x6B // Edit E (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC108_EDITF			0x6C // Edit F (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC109_EQ_LOW			0x6D // EQ Low: 0... 32 (-16... +16dB)
#define MIDI_BEHRINGER_DSP2024_CC110_EQ_HIGH		0x6E // EQ High: 0... 32 (-16... +16dB)
#define MIDI_BEHRINGER_DSP2024_CC111_MIX			0x69 // Mix (Depends on effect)
#define MIDI_BEHRINGER_DSP2024_CC112_STORE			0x70 // Store: 0... 99 (U001... U100)
#define MIDI_BEHRINGER_DSP2024_CC113_INOUT			0x71 // In/Out: (Bypass/0... 100%)
#define MIDI_BEHRINGER_DSP2024_CC114_COMBINATION	0x72 // Combination Mode: 0... 2 (Serial 1, Serial2, Parallel)
#define MIDI_BEHRINGER_DSP2024_CC115_INPUTMODE		0x73 // 0, 1 (Mono, Stereo)
#define MIDI_BEHRINGER_DSP2024_CC116_EXTINTMIX		0x74 // 0, 1 (External, Internal)

// Zoom Studio 1204 MIDI Controllers Numbers

#define MIDI_ZOOM_STUDIO1204_CC008_MIXLEVEL			0x08 // Mix Level
#define MIDI_ZOOM_STUDIO1204_CC080_EFFECT_ONOFF		0x50 // Effect On/Off: <64 on, >63 off
#define MIDI_ZOOM_STUDIO1204_CC084_PATTERNSELECT	0x54 // Pattern Select
#define MIDI_ZOOM_STUDIO1204_CC085_EDIT1			0x55 // Edit 1
#define MIDI_ZOOM_STUDIO1204_CC086_EDIT2			0x56 // Edit 2
#define MIDI_ZOOM_STUDIO1204_CC087_EQ_LOW			0x57 // EQ Low
#define MIDI_ZOOM_STUDIO1204_CC088_EQ_HIGH			0x58 // EQ High
#define MIDI_ZOOM_STUDIO1204_CC091_EFFECT_ONOFF		0x5B // Effect On/Off: <64 on, >63 off

#define MIDI_TO_FLOAT(i)	((float)(i) * 0.00787401574803149606299212598425197f)
#define FLOAT_TO_MIDI(i)	((char)((i) * 127.0f))
#define MIDI_TO_DOUBLE(i)	((double)(i) * 0.00787401574803149606299212598425197d)
#define DOUBLE_TO_MIDI(i)	((char)((i) * 127.0d))

#define MIDI_MAX_NOTES			128
#define MIDI_MAX_CC				128
#define MIDI_MAX_CHANNELS		 16

//////////////////////////////////////////////////////////////////////////////
//         Table 4: Summary of MIDI Note Numbers for Different Octaves
// (adapted from "MIDI by the Numbers" by D. Valenti - Electronic Musician 2/88)
//               Updated 1995 By the MIDI Manufacturers Association
//
//Octave||                     Note Numbers
//   #  ||
//      || C   | C#  | D   | D#  | E   | F   | F#  | G   | G#  | A   | A#  | B
//------------------------------------------------------------------------------
//  -1  ||   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7 |   8 |   9 |  10 |  11
//   0  ||  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  20 |  21 |  22 |  23
//   1  ||  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  32 |  33 |  34 |  35
//   2  ||  36 |  37 |  38 |  39 |  40 |  41 |  42 |  43 |  44 |  45 |  46 |  47
//   3  ||  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  58 |  59
//   4  ||  60 |  61 |  62 |  63 |  64 |  65 |  66 |  67 |  68 |  69 |  70 |  71
//   5  ||  72 |  73 |  74 |  75 |  76 |  77 |  78 |  79 |  80 |  81 |  82 |  83
//   6  ||  84 |  85 |  86 |  87 |  88 |  89 |  90 |  91 |  92 |  93 |  94 |  95
//   7  ||  96 |  97 |  98 |  99 | 100 | 101 | 102 | 103 | 104 | 105 | 106 | 107
//   8  || 108 | 109 | 110 | 111 | 112 | 113 | 114 | 115 | 116 | 117 | 118 | 119
//   9  || 120 | 121 | 122 | 123 | 124 | 125 | 126 | 127 |
//////////////////////////////////////////////////////////////////////////////

enum MidiNoteStatus {NOTE_NULL = -1, NOTE_OFF, NOTE_ON};

struct MidiNote {
	enum MidiNoteStatus Status;
	int Pitch;
	int Channel;
	int Velocity;
	MidiNote() : Status(NOTE_NULL), Pitch(0), Channel(0), Velocity(0) {}
	MidiNote(MidiNoteStatus _status, int _pitch, int _channel, int _velocity) : Status(_status), Pitch(_pitch), Channel(_channel), Velocity(_velocity) {}
	~MidiNote() {}
};


