

// Sets the TPM0 timer to zero and starts it.
// It will count to 'time' then an overflow occurs.
// time: in us.
void clearTimer(uint16_t time) {
  // Set time when overflow occurs.
  TPM0_MOD = time; 
        
  // Restart timer 0  
  TPM0_CNT = 0; 
  // Clear overflow
  while(TPM0_SC & FTM_SC_TOF)
    TPM0_SC |= FTM_SC_TOF; // spin wait
}
