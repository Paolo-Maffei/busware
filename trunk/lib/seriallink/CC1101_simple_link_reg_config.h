// Sync word qualifier mode = 30/32 sync word bits detected 
// CRC autoflush = true 
// Channel spacing = 199.951172 
// Data format = Normal mode 
// Data rate = 249.939 
// RX filter BW = 541.666667 
// PA ramping = false 
// Preamble count = 4 
// Whitening = true 
// Address config = No address check 
// Carrier frequency = 863.499847 
// Device address = 0 
// TX power = 0 
// Manchester enable = true 
// CRC enable = true 
// Deviation = 126.953125 
// Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word 
// Packet length = 255 
// Modulation format = GFSK 
// Base frequency = 863.499847 
// Modulated = true 
// Channel number = 0 

#include "board.h"

#ifdef NO_PGM
#define CFG(index)  CC1101_CFG[index]
static const uint8_t CC1101_CFG[] = {
#else
#include <avr/pgmspace.h>
#define CFG(index)  pgm_read_byte(&CC1101_CFG[index])
static const uint8_t PROGMEM CC1101_CFG[] = {
#endif
  0x00, 0x07, //GDO2 Output Pin Configuration,
  0x02, 0x09, //GDO0 Output Pin Configuration,
  0x07, 0x0C, //Packet Automation Control,
  0x0B, 0x0C, //Frequency Synthesizer Control,
  0x0D, 0x21, //Frequency Control Word, High Byte,
  0x0E, 0x36, //Frequency Control Word, Middle Byte,
  0x0F, 0x27, //Frequency Control Word, Low Byte,
  0x10, 0x2D, //Modem Configuration,
  0x11, 0x3B, //Modem Configuration,
  0x12, 0x1B, //Modem Configuration,
  0x15, 0x62, //Modem Deviation Setting,
  0x17, 0x3f, //CCA both - always back to RX
  0x18, 0x18, //Main Radio Control State Machine Configuration,
  0x19, 0x1D, //Frequency Offset Compensation Configuration,
  0x1A, 0x1C, //Bit Synchronization Configuration,
  0x1B, 0xC7, //AGC Control,
  0x1C, 0x00, //AGC Control,
  0x1D, 0xB0, //AGC Control,
  0x20, 0xFB, //Wake On Radio Control,
  0x21, 0xB6, //Front End RX Configuration,
  0x23, 0xEA, //Frequency Synthesizer Calibration,
  0x24, 0x2A, //Frequency Synthesizer Calibration,
  0x25, 0x00, //Frequency Synthesizer Calibration,
  0x26, 0x1F, //Frequency Synthesizer Calibration,
  0x2E, 0x09, //Various Test Settings,
  0xff
};
