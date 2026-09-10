// PULSING HEART - 2 frames (shrunk and expanded like a real heartbeat)
uint8_t pulsingHeartFrames[2][8] = {
  // Frame 1 - Shrunk/relaxed
  {
    0b00000000,
    0b00111100,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000,
    0b00000000
  },
  // Frame 2 - Expanded/pumping
  {
    0b01100110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000
  }
};
