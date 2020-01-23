  SED

  LDA #$05
  CLC
  ADC #$05
  OUT A

  LDA #$09
  CLC
  ADC #$10
  OUT A

  SEC
  LDA #$58
  ADC #$46
  OUT A

  CLC
  LDA #$12
  ADC #$34
  OUT A

  CLC
  LDA #$15
  ADC #$26
  OUT A

  CLC
  LDA #$81
  ADC #$92
  OUT A

  SEC
  LDA #$46
  SBC #$12
  OUT A

  SEC
  LDA #$40
  SBC #$13
  OUT A

  CLC
  LDA #$32
  SBC #$02
  OUT A

  SEC
  LDA #$12
  SBC #$21
  OUT A

  SEC
  LDA #$21
  SBC #$34
  OUT A

  CLC
  LDA #$90
  ADC #$90
  OUT A

  SEC
  LDA #$01
  SBC #$01
  OUT A

  CLC
  LDA #$99
  ADC #$01
  OUT A
