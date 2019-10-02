
// Setup timer.
void setupTimer() {
  // Setup main clock generator
  MCG_C1 &= ~0b010; // Disable IRCLK
  MCG_SC &= ~0b01110; // Divider = 1 => 4MHz
  MCG_C2 |= 0b001;  // IRCS. fast internal reference clock enabled
  MCG_C1 |= 0b010;  // IRCLKEN = enabled
  
  // Clock source
  SIM_SOPT2 |= 0b00000011000000000000000000000000;  // MCGIRCLK

  // Prescaler: 4 -> 4MHz/4 = 1MHz => 1us
  while (0 != (TPM0_SC & 0b00001000))
    TPM0_SC = 0;  // spin wait 
  TPM0_SC |= 0b00000010;  // Prescaler /4 = 1Mhz
  TPM0_SC |= 0b00001000;  // Enable timer
}


// Sets the TPM0 timer to zero and starts it.
// It will count to 'time' then an overflow occurs.
// time: in us.
void clearTimer(uint16_t time) {
  // Set time when overflow occurs.
  TPM0_MOD = (uint32_t)time; 
        
  // Restart timer 0  
  TPM0_CNT = 0; 
  // Clear overflow
  while(TPM0_SC & FTM_SC_TOF)
    TPM0_SC |= FTM_SC_TOF; // spin wait
}


// Returns the time since clearTimer.
// In us.
inline uint16_t GetTime()
{
  return TPM0_CNT;
}


// Returns true if the timer has overflown.
// TPM0_CNT >= TPM0_MOD.
inline bool isTimerOverflow() {
  return ((TPM0_SC & FTM_SC_TOF) != 0);
}
