
let cartridgeReaderEl = document.querySelector('#cartridgeReader')
let cartridgeDumpEl = document.querySelector('#cartridgeDump')

let fileReader = new FileReader()

let cartridgeBytes = null

cartridgeReaderEl.onchange = ev => {
  fileReader.readAsArrayBuffer( cartridgeReaderEl.files[0] )
}

fileReader.onloadend = () => {
  cartridgeBytes = new Uint8Array(fileReader.result) 
}

let regA=0
let regX=0
let regY=0
let regS=0
let regPC=0///
let flagC=0
let flagZ=0
let flagI=0
let flagD=0
let flagB=0
let flagV=0
let flagN=0

let RamBytes = new Uint8Array(128) // TODO init 0 or garbage?

let /*00*/      VSYNC   // ......1.  vertical sync set-clear
let /*01*/      VBLANK  // 11....1.  vertical blank set-clear
let /*02*/      WSYNC   // <strobe>  wait for leading edge of horizontal blank
let /*03*/      RSYNC   // <strobe>  reset horizontal sync counter
let /*04*/      NUSIZ0  // ..111111  number-size player-missile 0
let /*05*/      NUSIZ1  // ..111111  number-size player-missile 1
let /*06*/      COLUP0  // 1111111.  color-lum player 0 and missile 0
let /*07*/      COLUP1  // 1111111.  color-lum player 1 and missile 1
let /*08*/      COLUPF  // 1111111.  color-lum playfield and ball
let /*09*/      COLUBK  // 1111111.  color-lum background
let /*0A*/      CTRLPF  // ..11.111  control playfield ball size & collisions
let /*0B*/      REFP0   // ....1...  reflect player 0
let /*0C*/      REFP1   // ....1...  reflect player 1
let /*0D*/      PF0     // 1111....  playfield register byte 0
let /*0E*/      PF1     // 11111111  playfield register byte 1
let /*0F*/      PF2     // 11111111  playfield register byte 2
let /*10*/      RESP0   // <strobe>  reset player 0
let /*11*/      RESP1   // <strobe>  reset player 1
let /*12*/      RESM0   // <strobe>  reset missile 0
let /*13*/      RESM1   // <strobe>  reset missile 1
let /*14*/      RESBL   // <strobe>  reset ball
let /*15*/      AUDC0   // ....1111  audio control 0
let /*16*/      AUDC1   // ....1111  audio control 1
let /*17*/      AUDF0   // ...11111  audio frequency 0
let /*18*/      AUDF1   // ...11111  audio frequency 1
let /*19*/      AUDV0   // ....1111  audio volume 0
let /*1A*/      AUDV1   // ....1111  audio volume 1
let /*1B*/      GRP0    // 11111111  graphics player 0
let /*1C*/      GRP1    // 11111111  graphics player 1
let /*1D*/      ENAM0   // ......1.  graphics (enable) missile 0
let /*1E*/      ENAM1   // ......1.  graphics (enable) missile 1
let /*1F*/      ENABL   // ......1.  graphics (enable) ball
let /*20*/      HMP0    // 1111....  horizontal motion player 0
let /*21*/      HMP1    // 1111....  horizontal motion player 1
let /*22*/      HMM0    // 1111....  horizontal motion missile 0
let /*23*/      HMM1    // 1111....  horizontal motion missile 1
let /*24*/      HMBL    // 1111....  horizontal motion ball
let /*25*/      VDELP0  // .......1  vertical delay player 0
let /*26*/      VDELP1  // .......1  vertical delay player 1
let /*27*/      VDELBL  // .......1  vertical delay ball
let /*28*/      RESMP0  // ......1.  reset missile 0 to player 0
let /*29*/      RESMP1  // ......1.  reset missile 1 to player 1
let /*2A*/      HMOVE   // <strobe>  apply horizontal motion
let /*2B*/      HMCLR   // <strobe>  clear horizontal motion registers
let /*2C*/      CXCLR   // <strobe>  clear collision latches
let /*30*/      CXM0P   // 11......  read collision M0-P1, M0-P0 (Bit 7,6)
let /*31*/      CXM1P   // 11......  read collision M1-P0, M1-P1
let /*32*/      CXP0FB  // 11......  read collision P0-PF, P0-BL
let /*33*/      CXP1FB  // 11......  read collision P1-PF, P1-BL
let /*34*/      CXM0FB  // 11......  read collision M0-PF, M0-BL
let /*35*/      CXM1FB  // 11......  read collision M1-PF, M1-BL
let /*36*/      CXBLPF  // 1.......  read collision BL-PF, unused
let /*37*/      CXPPMM  // 11......  read collision P0-P1, M0-M1
let /*38*/      INPT0   // 1.......  read pot port
let /*39*/      INPT1   // 1.......  read pot port
let /*3A*/      INPT2   // 1.......  read pot port
let /*3B*/      INPT3   // 1.......  read pot port
let /*3C*/      INPT4   // 1.......  read input
let /*3D*/      INPT5   // 1.......  read input
let /*80..FF*/  RAM     // 11111111  128 bytes RAM (in PIA chip) for variables and stack
let /*0280*/    SWCHA   // 11111111  Port A; input or output  (read or write)
let /*0281*/    SWACNT  // 11111111  Port A DDR, 0= input, 1=output
let /*0282*/    SWCHB   // 11111111  Port B; console switches (read only)
let /*0283*/    SWBCNT  // 11111111  Port B DDR (hardwired as input)
let /*0284*/    INTIM   // 11111111  Timer output (read only)
let /*0285*/    INSTAT  // 11......  Timer Status (read only, undocumented)
let /*0294*/    TIM1T   // 11111111  set 1 clock interval (838 nsec/interval)
let /*0295*/    TIM8T   // 11111111  set 8 clock interval (6.7 usec/interval)
let /*0296*/    TIM64T  // 11111111  set 64 clock interval (53.6 usec/interval)
let /*0297*/    T1024T  // 01111111  set 1024 clock interval (858.2 usec/interval)


let currentInst = {opcode: 'NOP', addressingMode: 'Implied', operand: 0}
let executeAfter = 1

/* Instruction::
opcode: TAY TAX TSX TYA TXA TXS LDA LDX LDY STA STX STY PHA PHP PLA PLP ADC SBC AND EOR ORA CMP CPX CPY BIT INC INX INY DEC DEX DEY ASL LSR ROL ROR JMP JSR RTI RTS BPL BMI BVC BVS BCC BLT BCS BGE BNE BZC BEQ BZS BRK CLC CLI CLD CLV SEC SEI SED NOP
addressingMode: Implied Immediate ZeroPage ZeroPageX ZeroPageY Absolute AbsoluteX AbsoluteY IndirectX IndirectY
operand: (number)
*/


setInterval(() => {
  for (let colorClock = 0; colorClock<262*228; colorClock++) {
    if (colorClock%3==0) {
      executeAfter--
      if (executeAfter==0) {
        executeInst()
      }
    }
  }
}, 1/60);

function executeInst() {

  if (opcode=='TAY') {
    regY = regA
  }
  else if (opcode=='TAX') {
    regX = regA
  }
  else if (opcode=='TSX') {
    regX = regS
  }
  else if (opcode=='TYA') {
    regA = regY
  }
  else if (opcode=='TXA') {
    regA = regX
  }
  else if (opcode=='TXS') {
    regS = regX
  }
  else if (opcode=='LDA') {
    regA = loadValueUsingInst(currentInst)
  }
  else if (opcode=='LDX') {
    regX = loadValueUsingInst(currentInst)
  }
  else if (opcode=='LDY') {
    regY = loadValueUsingInst(currentInst)
  }
  else if (opcode=='STA') {
    storeValueUsingInst(currentInst, regA)
  }
  else if (opcode=='STX') {
    storeValueUsingInst(currentInst, regX)
  }
  else if (opcode=='STY') {
    storeValueUsingInst(currentInst, regY)
  }
  else if (opcode=='PHA') {

  }
  else if (opcode=='PHP') {

  }
  else if (opcode=='PLA') {

  }
  else if (opcode=='PLP') {

  }
  else if (opcode=='ADC') {
    regA += loadValueUsingInst(currentInst) + flagC
    regA = normalize(regA, 8)
  }
  else if (opcode=='SBC') {
    regA += flagC - 1 - loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
  }
  else if (opcode=='AND') {
    regA &= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
  }
  else if (opcode=='EOR') {
    regA ^= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
  }
  else if (opcode=='ORA') {
    regA |= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
  }
  else if (opcode=='CMP') {
    /////
  }
  else if (opcode=='CPX') {

  }
  else if (opcode=='CPY') {

  }
  else if (opcode=='BIT') {

  }
  else if (opcode=='INC') {

  }
  else if (opcode=='INX') {

  }
  else if (opcode=='INY') {

  }
  else if (opcode=='DEC') {

  }
  else if (opcode=='DEX') {

  }
  else if (opcode=='DEY') {

  }
  else if (opcode=='ASL') {

  }
  else if (opcode=='LSR') {

  }
  else if (opcode=='ROL') {

  }
  else if (opcode=='ROR') {

  }
  else if (opcode=='JMP') {

  }
  else if (opcode=='JSR') {

  }
  else if (opcode=='RTI') {

  }
  else if (opcode=='RTS') {

  }
  else if (opcode=='BPL') {

  }
  else if (opcode=='BMI') {

  }
  else if (opcode=='BVC') {

  }
  else if (opcode=='BVS') {

  }
  else if (opcode=='BCC') {

  }
  else if (opcode=='BLT') {

  }
  else if (opcode=='BCS') {

  }
  else if (opcode=='BGE') {

  }
  else if (opcode=='BNE') {

  }
  else if (opcode=='BZC') {

  }
  else if (opcode=='BEQ') {

  }
  else if (opcode=='BZS') {

  }
  else if (opcode=='BRK') {

  }
  else if (opcode=='CLC') {

  }
  else if (opcode=='CLI') {

  }
  else if (opcode=='CLD') {

  }
  else if (opcode=='CLV') {

  }
  else if (opcode=='SEC') {

  }
  else if (opcode=='SEI') {

  }
  else if (opcode=='SED') {

  }
  else if (opcode=='NOP') {

  }
  
}

function getByteFromMemory(addr, triggerIO) {

}

function setByteToMemory(addr, value, triggerIO) {

}

function loadValueUsingInst(inst) {
  if (inst.addressingMode=='Immediate') {
    return inst.operand
  }
  else if (inst.addressingMode=='ZeroPage') {
    return getByteFromMemory(inst.operand, true)
  }
  else if (inst.addressingMode=='ZeroPageX') {
    return getByteFromMemory((inst.operand+regX)%256, true)
  }
  else if (inst.addressingMode=='ZeroPageY') {
    return getByteFromMemory((inst.operand+regY)%256, true)
  }
  else if (inst.addressingMode=='Absolute') {
    return getByteFromMemory(inst.operand, true)
  }
  else if (inst.addressingMode=='AbsoluteX') {
    if (inst.operand%256+regX >= 256) {
      getByteFromMemory(inst.operand+regX-256, true)
    }
    return getByteFromMemory(inst.operand+regX, true)
  }
  else if (inst.addressingMode=='AbsoluteY') {
    if (inst.operand%256+regY >= 256) {
      getByteFromMemory(inst.operand+regY-256, true)
    }
    return getByteFromMemory(inst.operand+regY, true)
  }
  else if (inst.addressingMode=='IndirectX') {
    let addr = getByteFromMemory((inst.operand+regX)%256, false) +
      getByteFromMemory((inst.operand+regX)%256+1, false) << 8
    return getByteFromMemory(addr, true)
  }
  else if (inst.addressingMode=='IndirectY') {
    let addr = getByteFromMemory(inst.operand, false) +
      getByteFromMemory(inst.operand+1, false) << 8
    if (addr%256+regY >= 256) {
      getByteFromMemory(addr+regY-256, true)
    }
    return getByteFromMemory(addr+regY, true)
  }
  else {
    unreachable()
  }
}

function storeValueUsingInst(inst, value) {
  if (inst.addressingMode=='Immediate') {
    unreachable()
  }
  else if (inst.addressingMode=='ZeroPage') {
    setByteToMemory(inst.operand, value, true)
  }
  else if (inst.addressingMode=='ZeroPageX') {
    setByteToMemory((inst.operand+regX)%256, value, true)
  }
  else if (inst.addressingMode=='ZeroPageY') {
    setByteToMemory((inst.operand+regY)%256, value, true)
  }
  else if (inst.addressingMode=='Absolute') {
    setByteToMemory(inst.operand, value, true)
  }
  else if (inst.addressingMode=='AbsoluteX') {
    setByteToMemory(inst.operand+regX, value, true)
  }
  else if (inst.addressingMode=='AbsoluteY') {
    setByteToMemory(inst.operand+regY, value, true)
  }
  else if (inst.addressingMode=='IndirectX') {
    let addr = getByteFromMemory((inst.operand+regX)%256, false) + // TODO: are these true or false?
      getByteFromMemory((inst.operand+regX)%256+1, false) << 8
    setByteToMemory(addr, value, true)
  }
  else if (inst.addressingMode=='IndirectY') {
    let addr = getByteFromMemory(inst.operand, false) +
      getByteFromMemory(inst.operand+1, false) << 8
    setByteToMemory(addr+regY, value, true)
  }
  else {
    unreachable()
  }
}

function unreachable() {
  console.error('unreachable')
}

function normalize(val, bitLength) {
  let max = 2**bitLength
  return (val % max + max) % max
}