# 01/05/2020
 + Initial version
# 01/19/2020
 + Add load bin
 + add notification virtuals
 + add breakpoints and jump points
# 01/26/2020
 + fix ASL, ROL, ROR, LSR memory update
 + fix SBC
 + Add more OUT support for registers, memory and strings
 + Add ORG (set origin) for assembler
 + Add DB (define bytes) for assembler
 + Move parser to separate class
 + Allow suppress processing if debugger accessing memory
# 02/01/2020
 + catch recursive calls to IRQ/NMI
 + add tick virtual for instruction ticks
 + support next instruction
 + optional update of scrollbar for memory and instructions in debugger
