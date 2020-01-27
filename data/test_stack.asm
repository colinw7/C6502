  LDX #$01
  OUT X
 
  TXS
  OUT SP

  LDX #$02
  OUT X

  TSX
  OUT X

  LDA #$FF
  OUT A
  PHA

  LDA #$00
  OUT A
  PLP
  OUT A

  LDA #$53
  OUT A
  PHP

  LDA #$21
  OUT A
  PLA
  OUT A
