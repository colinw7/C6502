  CLC
  LDA #$00
  SBC #$00
  OUT AF

  CLC
  LDA #$80
  SBC #$00
  OUT AF

  CLC
  LDA #$00
  SBC #$80
  OUT AF

  SEC
  LDA #$00
  SBC #$01
  OUT AF

  SEC
  LDA #$80
  SBC #$01
  OUT AF

  SEC
  LDA #$7F
  SBC #$FF
  OUT AF

  CLC
  LDA #$C0
  SBC #$40
  OUT AF
