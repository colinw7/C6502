  LDX #$00
  LDY #$00
  STX $00,Y
  INX
  INY
  STX $00,Y
  INX
  INY
  STX $00,Y
  INX
  INY
  STX $00,Y

  OUT X
  OUT Y

  LDA #$00
  OUT A

  LDA $01
  OUT A

  LDX #$01
  LDA $01,X
  OUT A

  LDA $0002
  OUT A

  LDA $0002,X
  OUT A

  BRK
