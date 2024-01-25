
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

let regA=0 // 8bit
let regX=0 // 8bit
let regY=0 // 8bit
let regS=0 // 8bit
let regPC=0 // 16bit
let flagC=0
let flagZ=0
let flagI=0
let flagD=0
let flagB=0
let flagV=0
let flagN=0

function getRegP() {
  return flagN<<7 | flagV<<6 | 1<<5 | flagB<<4 | flagD<<3 | flagI<<2 | flagZ<<1 | flagC
}

function setRegP(value) {
  flagN = (value & (1 << 7)) !== 0
  flagV = (value & (1 << 6)) !== 0
  flagB = (value & (1 << 4)) !== 0
  flagD = (value & (1 << 3)) !== 0
  flagI = (value & (1 << 2)) !== 0
  flagZ = (value & (1 << 1)) !== 0
  flagC = (value & 1) !== 0
}

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


let mainLoop = setInterval(() => {
  for (let colorClock = 0; colorClock<262*228; colorClock++) {
    if (colorClock%3==0) {
      executeAfter--
      if (executeAfter==0) {
        executeInst()
        fetchDecodeNextInst()
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
    setByteToMemory(regS, regA, true)
    regS--
  }
  else if (opcode=='PHP') {
    setByteToMemory(regS, getRegP(), true)
    regS--
  }
  else if (opcode=='PLA') {
    regS++
    getByteFromMemory(regS, true)
  }
  else if (opcode=='PLP') {
    regS++
    setRegP( getByteFromMemory(regS, true) )
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
  else if (opcode=='BCS') {

  }
  else if (opcode=='BNE') {

  }
  else if (opcode=='BEQ') {

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

function fetchDecodeNextInst() {
  let firstByte = getByteFromMemory(regPC, true)

  let insts = [
    {firstByte: 0xA8,	opcode: 'TAY',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xAA,	opcode: 'TAX',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xBA,	opcode: 'TSX',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x98,	opcode: 'TYA',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x8A,	opcode: 'TXA',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x9A,	opcode: 'TXS',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xA9,	opcode: 'LDA',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xA2,	opcode: 'LDX',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xA0,	opcode: 'LDY',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xA5,	opcode: 'LDA',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xB5,	opcode: 'LDA',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0xAD,	opcode: 'LDA',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xBD,	opcode: 'LDA',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0xB9,	opcode: 'LDA',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0xA1,	opcode: 'LDA',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0xB1,	opcode: 'LDA',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0xA6,	opcode: 'LDX',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xB6,	opcode: 'LDX',	addressingMode: 'ZeroPageY',	cycles: 4},
    {firstByte: 0xAE,	opcode: 'LDX',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xBE,	opcode: 'LDX',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0xA4,	opcode: 'LDY',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xB4,	opcode: 'LDY',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0xAC,	opcode: 'LDY',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xBC,	opcode: 'LDY',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0x85,	opcode: 'STA',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x95,	opcode: 'STA',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x8D,	opcode: 'STA',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x9D,	opcode: 'STA',	addressingMode: 'AbsoluteX',	cycles: 5},
    {firstByte: 0x99,	opcode: 'STA',	addressingMode: 'AbsoluteY',	cycles: 5},
    {firstByte: 0x81,	opcode: 'STA',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0x91,	opcode: 'STA',	addressingMode: 'IndirectY',	cycles: 6},
    {firstByte: 0x86,	opcode: 'STX',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x96,	opcode: 'STX',	addressingMode: 'ZeroPageY',	cycles: 4},
    {firstByte: 0x8E,	opcode: 'STX',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x84,	opcode: 'STY',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x94,	opcode: 'STY',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x8C,	opcode: 'STY',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x48,	opcode: 'PHA',	addressingMode: 'Implied',	cycles: 3},
    {firstByte: 0x08,	opcode: 'PHP',	addressingMode: 'Implied',	cycles: 3},
    {firstByte: 0x68,	opcode: 'PLA',	addressingMode: 'Implied',	cycles: 4},
    {firstByte: 0x28,	opcode: 'PLP',	addressingMode: 'Implied',	cycles: 4},
    {firstByte: 0x69,	opcode: 'ADC',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0x65,	opcode: 'ADC',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x75,	opcode: 'ADC',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x6D,	opcode: 'ADC',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x7D,	opcode: 'ADC',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0x79,	opcode: 'ADC',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0x61,	opcode: 'ADC',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0x71,	opcode: 'ADC',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0xE9,	opcode: 'SBC',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xE5,	opcode: 'SBC',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xF5,	opcode: 'SBC',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0xED,	opcode: 'SBC',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xFD,	opcode: 'SBC',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0xF9,	opcode: 'SBC',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0xE1,	opcode: 'SBC',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0xF1,	opcode: 'SBC',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0x29,	opcode: 'AND',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0x25,	opcode: 'AND',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x35,	opcode: 'AND',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x2D,	opcode: 'AND',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x3D,	opcode: 'AND',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0x39,	opcode: 'AND',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0x21,	opcode: 'AND',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0x31,	opcode: 'AND',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0x49,	opcode: 'EOR',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0x45,	opcode: 'EOR',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x55,	opcode: 'EOR',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x4D,	opcode: 'EOR',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x5D,	opcode: 'EOR',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0x59,	opcode: 'EOR',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0x41,	opcode: 'EOR',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0x51,	opcode: 'EOR',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0x09,	opcode: 'ORA',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0x05,	opcode: 'ORA',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x15,	opcode: 'ORA',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0x0D,	opcode: 'ORA',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x1D,	opcode: 'ORA',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0x19,	opcode: 'ORA',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0x01,	opcode: 'ORA',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0x11,	opcode: 'ORA',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0xC9,	opcode: 'CMP',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xC5,	opcode: 'CMP',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xD5,	opcode: 'CMP',	addressingMode: 'ZeroPageX',	cycles: 4},
    {firstByte: 0xCD,	opcode: 'CMP',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xDD,	opcode: 'CMP',	addressingMode: 'AbsoluteX',	cycles: 4},
    {firstByte: 0xD9,	opcode: 'CMP',	addressingMode: 'AbsoluteY',	cycles: 4},
    {firstByte: 0xC1,	opcode: 'CMP',	addressingMode: 'IndirectX',	cycles: 6},
    {firstByte: 0xD1,	opcode: 'CMP',	addressingMode: 'IndirectY',	cycles: 5},
    {firstByte: 0xE0,	opcode: 'CPX',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xE4,	opcode: 'CPX',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xEC,	opcode: 'CPX',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xC0,	opcode: 'CPY',	addressingMode: 'Immediate',	cycles: 2},
    {firstByte: 0xC4,	opcode: 'CPY',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0xCC,	opcode: 'CPY',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0x24,	opcode: 'BIT',	addressingMode: 'ZeroPage',	cycles: 3},
    {firstByte: 0x2C,	opcode: 'BIT',	addressingMode: 'Absolute',	cycles: 4},
    {firstByte: 0xE6,	opcode: 'INC',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0xF6,	opcode: 'INC',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0xEE,	opcode: 'INC',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0xFE,	opcode: 'INC',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0xE8,	opcode: 'INX',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xC8,	opcode: 'INY',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xC6,	opcode: 'DEC',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0xD6,	opcode: 'DEC',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0xCE,	opcode: 'DEC',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0xDE,	opcode: 'DEC',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0xCA,	opcode: 'DEX',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x88,	opcode: 'DEY',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x0A,	opcode: 'ASL',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x06,	opcode: 'ASL',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0x16,	opcode: 'ASL',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0x0E,	opcode: 'ASL',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0x1E,	opcode: 'ASL',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0x4A,	opcode: 'LSR',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x46,	opcode: 'LSR',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0x56,	opcode: 'LSR',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0x4E,	opcode: 'LSR',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0x5E,	opcode: 'LSR',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0x2A,	opcode: 'ROL',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x26,	opcode: 'ROL',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0x36,	opcode: 'ROL',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0x2E,	opcode: 'ROL',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0x3E,	opcode: 'ROL',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0x6A,	opcode: 'ROR',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x66,	opcode: 'ROR',	addressingMode: 'ZeroPage',	cycles: 5},
    {firstByte: 0x76,	opcode: 'ROR',	addressingMode: 'ZeroPageX',	cycles: 6},
    {firstByte: 0x6E,	opcode: 'ROR',	addressingMode: 'Absolute',	cycles: 6},
    {firstByte: 0x7E,	opcode: 'ROR',	addressingMode: 'AbsoluteX',	cycles: 7},
    {firstByte: 0x4C,	opcode: 'JMP',	addressingMode: 'Immediate2',	cycles: 3},
    {firstByte: 0x6C,	opcode: 'JMP',	addressingMode: 'Absolute',	cycles: 5},
    {firstByte: 0x20,	opcode: 'JSR',	addressingMode: 'Immediate2',	cycles: 6},
    {firstByte: 0x40,	opcode: 'RTI',	addressingMode: 'Implied',	cycles: 6},
    {firstByte: 0x60,	opcode: 'RTS',	addressingMode: 'Implied',	cycles: 6},
    {firstByte: 0x10,	opcode: 'BPL',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0x30,	opcode: 'BMI',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0x50,	opcode: 'BVC',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0x70,	opcode: 'BVS',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0x90,	opcode: 'BCC',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0xB0,	opcode: 'BCS',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0xD0,	opcode: 'BNE',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0xF0,	opcode: 'BEQ',	addressingMode: 'Immediate2',	cycles: 2},
    {firstByte: 0x00,	opcode: 'BRK',	addressingMode: 'Implied',	cycles: 7},
    {firstByte: 0x18,	opcode: 'CLC',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x58,	opcode: 'CLI',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xD8,	opcode: 'CLD',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xB8,	opcode: 'CLV',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x38,	opcode: 'SEC',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0x78,	opcode: 'SEI',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xF8,	opcode: 'SED',	addressingMode: 'Implied',	cycles: 2},
    {firstByte: 0xEA,	opcode: 'NOP',	addressingMode: 'Implied',	cycles: 2},
  ]

  // TODO: illegal insts, including NOPS with different first bytes
  
  let addressingModeToOperandLength = {
    Implied: 0,
    Immediate: 1,
    Immediate2: 2,
    ZeroPage: 1,
    ZeroPageX: 1,
    ZeroPageY: 1,
    Absolute: 2,
    AbsoluteX: 2,
    AbsoluteY: 2,
    IndirectX: 1,
    IndirectY: 1
  }

  let inst
  for (let i = 0; i<insts.length; i++) {
    if (insts[i].firstByte == firstByte) {
      inst = insts[i]
      break
    }
  }
  if (!inst) {
    console.error(`Unknown opcode: ${firstByte}, pc:${regPC}`)
    clearInterval(mainLoop)
  }

  currentInst.opcode = inst.opcode
  currentInst.addressingMode = inst.addressingMode

  if (addressingModeToOperandLength[inst.addressingMode]==0) {
    currentInst.operand = 0
    regPC++
  }
  else if (addressingModeToOperandLength[inst.addressingMode]==1) {
    currentInst.operand = getByteFromMemory(regPC+1, true)
    // TODO: if pc is FFFF then this should return to 0 or what?
    regPC+=2
  }
  else if (addressingModeToOperandLength[inst.addressingMode]==2) {
    currentInst.operand = getByteFromMemory(regPC+1, true)<<8 | getByteFromMemory(regPC+2, true)
    regPC+=3
  }
  else {
    unreachable()
  }

  // TODO: for jumps and branches pc needs to change

  executeAfter = inst.cycles //// TODO: test the page boundary
  if (['AbsoluteX', 'AbsoluteY', 'IndirectY'].includes(inst.addressingMode) &&
  !['STA','STX','STY','INC','INX','INY','DEC','DEX','DEY','ASL','LSR','ROL','ROR'].includes(inst.opcode)) {
    executeAfter++
  }
}


// cycle incrs
// -absoluteX, absoluteY, indirectY (NOT in ST*, INC, DEC, shifts)
// branch: no branch 2, yes branch 3, yes branch page cross 4
// note: jmp [nnnn] cannot cross page boundary

// dummy read
// happens whenn absoluteX, absoluteY, indirectY (same as above) 

// dummy write
// INC, DEC, shift, rotates
// - we assume dummy write is same as final write, so we just do store twice
