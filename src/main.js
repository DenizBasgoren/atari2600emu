
let cartridgeReaderEl = document.querySelector('#cartridgeReader')

let fileReader = new FileReader()

let cartridgeBytes = null

let cartridgeMappingIndex = 0 // assume 0 for now. TODO: detect mapping
let cartridgeMapping = [ // just copy for now, later needs to be dynamic
  newRomEntry(0,        0xFFF,    0,        -1,       -1,       true)
]
// also note that upon copying here, expansion rams should have memory as a field

cartridgeReaderEl.onchange = ev => {
  fileReader.readAsArrayBuffer( cartridgeReaderEl.files[0] )
}

fileReader.onloadend = () => {
  cartridgeBytes = new Uint8Array(fileReader.result) 
}

let tvEl = document.querySelector('#TV')
let tvCtx = tvEl.getContext('2d')

/// tv type
let tvType = 'ntsc'
let canvasWidth = 160 // doesnt change
let canvasHeight = 262 // ntsc=262, pal,secam,mono=312
let frameRate = 60 // ntsc=60, pal,secam,mono=50 Hz
let tvTypeEl = document.querySelector('#tvType')
tvTypeEl.onchange = ev => {
  console.log(ev.target.value)
  tvType = ev.target.value
  if (tvType == 'ntsc') {
    canvasHeight = 262
    frameRate = 60
  }
  else if (tvType == 'pal' || tvType == 'secam' || tvType == 'mono') {
    canvasHeight = 312
    frameRate = 50
  }
  clearInterval(mainLoop)
  tvEl.width = canvasWidth
  tvEl.height = canvasHeight
  mainLoop = setInterval(mainFn, 1/frameRate)
}

/// controllers
let controllerType = 'joystick' // joystick, joystick3, paddle, keypad
let controllerTypeEl = document.querySelector('#gameControllerType')
controllerTypeEl.onchange = ev => {
  console.log(ev.target.value)
  controllerType = ev.target.value
}

let swacnt = [0,0,0,0,0,0,0,0] // 0-input (joystick, paddle) 1-output (keypad)
let swbcnt = [0,0,0,0,0,0,0,0]
let latchedInputs = [0,0] // inpt4-5
let vblankInpt = [0,0] // vblank6-7

let pressedKeys = {}

// What happens if joysticks are connected and swcha is all output?

// KeyW KeyA.., Numpad0, Numpad9, NumpadDivide, NumpadMultiply
// Player0: wasdf, Player1: ijklh
window.onkeydown = ev => {
  pressedKeys[ev.code] = true
}

window.onkeyup = ev => {
  pressedKeys[ev.code] = false
}

window.onload = ev => {
  controllerType = 'joystick'
  controllerTypeEl.value = 'joystick'
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


let currentInst = {opcode: 'NOP', addressingMode: 'Implied', operand: 0}
let executeAfter = 1

/* Instruction::
opcode: TAY TAX TSX TYA TXA TXS LDA LDX LDY STA STX STY PHA PHP PLA PLP ADC SBC AND EOR ORA CMP CPX CPY BIT INC INX INY DEC DEX DEY ASL LSR ROL ROR JMP JSR RTI RTS BPL BMI BVC BVS BCC BLT BCS BGE BNE BZC BEQ BZS BRK CLC CLI CLD CLV SEC SEI SED NOP
addressingMode: Implied Immediate ZeroPage ZeroPageX ZeroPageY Absolute AbsoluteX AbsoluteY IndirectX IndirectY
operand: (number)
*/

// UNCOMMENT LATER
let machineCycleAfter = 0
let vsyncBecameOne = false
var mainLoop = setInterval(function mainFn() {
  for (let colorClock = 0; colorClock<100000; colorClock++) {

    if ( colorClock % 228 == 0 ) {
      isWaitingHsync = false
    }
    if (colorClock >= 228*canvasHeight) {
      // don't draw anything
    }
    else if (vblank || vsync) {
      // draw black dot
    }
    else if ( colorClock % 228 >= 68 ) {
      /// draw fn
    }

    if (machineCycleAfter==0) {
      machineCycleAfter = 3
      executeAfter--
      if (executeAfter==0 && !isWaitingHsync) {
        executeInst()
        fetchDecodeNextInst()
      }
      updateTimer()
    }
    machineCycleAfter--

    if (vsync && !vsyncBecameOne) { // vsync goes 0 to 1
      vsyncBecameOne = true
    }
    else if (!vsync && vsyncBecameOne) { // vsync goes 1 to 0
      break
    }
  }

  vsyncBecameOne = false
}, 1/60);

function executeInst() {
  let opcode = currentInst.opcode

  if (opcode=='TAY') {
    regY = regA
    updateNZ(regY)
  }
  else if (opcode=='TAX') {
    regX = regA
    updateNZ(regX)
  }
  else if (opcode=='TSX') {
    regX = regS
    updateNZ(regX)
  }
  else if (opcode=='TYA') {
    regA = regY
    updateNZ(regA)
  }
  else if (opcode=='TXA') {
    regA = regX
    updateNZ(regA)
  }
  else if (opcode=='TXS') {
    regS = regX
  }
  else if (opcode=='LDA') {
    regA = loadValueUsingInst(currentInst)
    updateNZ(regA)
  }
  else if (opcode=='LDX') {
    regX = loadValueUsingInst(currentInst)
    updateNZ(regX)
  }
  else if (opcode=='LDY') {
    regY = loadValueUsingInst(currentInst)
    updateNZ(regY)
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
    setByteToMemory(256+regS, regA)
    regS--
    normalize(regS, 8)
  }
  else if (opcode=='PHP') {
    setByteToMemory(256+regS, getRegP())
    regS--
    normalize(regS, 8)
  }
  else if (opcode=='PLA') {
    regS++
    normalize(regS, 8)
    regA = getByteFromMemory(256+regS, true)
    updateNZ(regA)
  }
  else if (opcode=='PLP') {
    regS++
    normalize(regS, 8)
    setRegP( getByteFromMemory(256+regS, true) )
  }
  else if (opcode=='ADC') {
    let arg2 = loadValueUsingInst(currentInst)
    if (flagD) {
      //// http://www.6502.org/tutorials/decimal_mode.html#A
      let regAlow = (regA & 0xF) + (arg2 & 0xF) + flagC
      if (regAlow >= 10) regAlow = ((regAlow + 6) & 0xF) + 0x10
      regA = (regA & 0xF0) + (arg2 & 0xF0) + regAlow
      if (regA >= 0xA0) regA += 0x60
    }
    else {
      regA += arg2 + flagC
    }
    flagC = regA >= 256
    flagV = regA > 383 || regA < 128
    regA = normalize(regA, 8)
    updateNZ(regA)
  }
  else if (opcode=='SBC') {
    let arg2 = loadValueUsingInst(currentInst)
    if (flagD) {
      //// http://www.6502.org/tutorials/decimal_mode.html#A
      let regAlow = (regA & 0xF) - (arg2 & 0xF) + flagC - 1
      if (regAlow < 0) regAlow = ((regAlow - 6) & 0xF) - 0x10
      regA = (regA & 0xF0) - (arg2 & 0xF0) + regAlow
      if (regA < 0) regA -= 0x60
    }
    else {
      regA += flagC - 1 - arg2
    }
    flagC = regA >= 0
    flagV = regA > 127 || regA < -128
    regA = normalize(regA, 8)
    updateNZ(regA)
  }
  else if (opcode=='AND') {
    regA &= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
    updateNZ(regA)
  }
  else if (opcode=='EOR') {
    regA ^= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
    updateNZ(regA)
  }
  else if (opcode=='ORA') {
    regA |= loadValueUsingInst(currentInst)
    regA = normalize(regA, 8)
    updateNZ(regA)
  }
  else if (opcode=='CMP') {
    let val = loadValueUsingInst(currentInst)
    updateNZ( normalize(regA-val, 8) )
    flagC = regA >= val
  }
  else if (opcode=='CPX') {
    let val = loadValueUsingInst(currentInst)
    updateNZ( normalize(regX-val, 8) )
    flagC = regX >= val
  }
  else if (opcode=='CPY') {
    let val = loadValueUsingInst(currentInst)
    updateNZ( normalize(regY-val, 8) )
    flagC = regY >= val
  }
  else if (opcode=='BIT') {
    let result = regA & loadValueUsingInst(currentInst)
    updateNZ(result)
    flagV = result & 64 != 0
  }
  else if (opcode=='INC') {
    let val = loadValueUsingInst(currentInst)
    val++
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='INX') {
    regX++
    normalize(regX, 8)
    updateNZ(regX)
  }
  else if (opcode=='INY') {
    regY++
    normalize(regY, 8)
    updateNZ(regY)
  }
  else if (opcode=='DEC') {
    let val = loadValueUsingInst(currentInst)
    val--
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='DEX') {
    regX--
    normalize(regX, 8)
    updateNZ(regX)
  }
  else if (opcode=='DEY') {
    regY--
    normalize(regY, 8)
    updateNZ(regY)
  }
  else if (opcode=='ASL') {
    let val = loadValueUsingInst(currentInst)
    flagC = val & 128 != 0
    val <<= 1
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='LSR') {
    let val = loadValueUsingInst(currentInst)
    flagC = val & 1
    val >>= 1
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='ROL') {
    let val = loadValueUsingInst(currentInst)
    let oldFlagC = flagC
    flagC = val & 128 != 0
    val <<= 1
    val += oldFlagC
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='ROR') {
    let val = loadValueUsingInst(currentInst)
    let oldFlagC = flagC
    flagC = val & 1
    val >>= 1
    val += oldFlagC * 128
    val = normalize(val, 8)
    updateNZ(val)
    storeValueUsingInst(currentInst, val)
  }
  else if (opcode=='JMP') {
    regPC = loadValueUsingInst(currentInst)
  }
  else if (opcode=='JSR') {
    setByteToMemory(256+regS, regPC>>8)
    regS--
    normalize(regS, 8)
    setByteToMemory(256+regS, regPC&256)
    regS--
    normalize(regS, 8)
    regPC = loadValueUsingInst(currentInst)
  }
  else if (opcode=='RTI') {
    regS++
    normalize(regS, 8)
    setRegP( getByteFromMemory(256+regS, true) )
    regS++
    normalize(regS, 8)
    let lowByte = getByteFromMemory(256+regS, true)
    regS++
    normalize(regS, 8)
    let highByte = getByteFromMemory(256+regS, true)
    regPC = lowByte | highByte<<8
  }
  else if (opcode=='RTS') {
    regS++
    normalize(regS, 8)
    let lowByte = getByteFromMemory(256+regS, true)
    regS++
    normalize(regS, 8)
    let highByte = getByteFromMemory(256+regS, true)
    regPC = lowByte | highByte<<8
  }
  else if (opcode=='BPL') {
    if (flagN==0) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BMI') {
    if (flagN==1) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BVC') {
    if (flagV==0) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BVS') {
    if (flagV==1) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BCC') {
    if (flagC==0) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BCS') {
    if (flagC==1) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BNE') {
    if (flagZ==0) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BEQ') {
    if (flagZ==1) {
      regPC += currentInst.operand // TODO: check if pc offset is right here
    }
  }
  else if (opcode=='BRK') {
    flagB=1
    flagI=1
    setByteToMemory(256+regS, regPC>>8)
    regS--
    normalize(regS, 8)
    setByteToMemory(256+regS, regPC&256)
    regS--
    normalize(regS, 8)
    setByteToMemory(256+regS, getRegP())
    regS--
    normalize(regS, 8)
    regPC = loadValueUsingInst({addressingMode: 'Absolute2', operand: 0xFFFE})
  }
  else if (opcode=='CLC') {
    flagC=0
  }
  else if (opcode=='CLI') {
    flagI=0
  }
  else if (opcode=='CLD') {
    flagD=0
  }
  else if (opcode=='CLV') {
    flagV=0
  }
  else if (opcode=='SEC') {
    flagC=1
  }
  else if (opcode=='SEI') {
    flagI=1
  }
  else if (opcode=='SED') {
    flagD=1
  }
  else if (opcode=='NOP') {

  }
  
}

let ram = Array(128).map(v => 0) // TODO init 0 or garbage?
let electronBeamX = 0
let electronBeamY = 0
let vsync = 0 // 0=off 1=on
let vblank = 0 // 0=off 1=on

let player0 = {
  color: 0, //
  luminance: 0, //
  position: 0, //
  positionAdjustment: 0, //
  sprite: [0,0,0,0,0,0,0,0], //
  isReflected: false, //
  numberSizeSetting: 0, //
}

let player1 = {
  color: 0, //
  luminance: 0, //
  position: 0, //
  positionAdjustment: 0, //
  sprite: [0,0,0,0,0,0,0,0], //
  isReflected: false, //
  numberSizeSetting: 0, //
}

let missile0 = {
  position: 0, //
  positionAdjustment: 0, //
  isPositionLockedOnPlayer: false, //
  isVisible: false, //
  size: 0, //
}

let missile1 = {
  position: 0, //
  positionAdjustment: 0, //
  isPositionLockedOnPlayer: false, //
  isVisible: false, //
  size: 0, //
}

let ball = {
  position: 0, //
  positionAdjustment: 0, //
  isVisible: false, //
  size: 0, //
}

let playfield = {
  sprite: [0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0],
  color: 0,
  luminance: 0,
  isReflected: false,
}

let background = {
  color: 0,
  luminance: 0
}

let isScoreMode = false
let isBallPriorityMode = false
let isWaitingHsync = false

let timer = {
  value: 0,
  interval: 0,
  internalCounter: 0,
  underflow: false,
}

function updateTimer() { // TODO test this
  if (timer.internalCounter)
  
  if (timer.underflow) {
    if (timer.value==0) {
      timer.value = 255
    }
    else {
      timer.value--
    }
  }
  else {
    timer.internalCounter--
    if (timer.internalCounter==0){
      timer.value--
    }
    if (timer.value==0) {
      timer.underflow = true
      timer.value = 255
    }
    else {
      timer.value--
    }
  }
}


function getByteFromMemory(addr, triggerIO) {
  // TODO handle 0x3F here

  if (addr & 0x1000) {
    // cartridge
    let addr = addr & 0xFFF
    let cm = cartridgeMapping
    for (let i = 0; i<cm.length; i++) {
      if (cm[i].triggerAddress == addr) {

        if (triggerIO) {
          // NOTE: we don't look at triggerValue for read operations.
          // This will need to be reconsidered for 3F banking.
          //
          cm[i].isActive = true
          
          for (let j = 0; j<cm.length; j++) {
            if (j==i) continue
            if (!cm[j].isActive) continue
  
            let a = cm[i].startFileOffset // the new region
            let b = cm[i].endFileOffset
            let c = cm[j].startFileOffset // the old region
            let d = cm[j].endFileOffset
  
            if ( !(b < c || d < a) ) {
              // they intersect
              cm[j].isActive = false
            }
          }

        }


        return 0 // we assume there's only one triggerAddress for each entry
      }
    }

    // no triggerAddress is targetted. So we do the normal operation
    for (let i = 0; i<cm.length; i++) {
      if (!cm[i].isActive) continue

      if (cm[i].type=='rom') {
        /*
        301-300+1 = 2
        end-start+1

        madr <= adr <= madr+len-1
        0        0,1    len=2

        400 madr
        5 len
        adr: 400,401,402,403,404
          madr <= adr <= madr+len-1
        */
        if ( isBetweenInclusive(addr, cm[i].mappedAddress, cm[i].mappedAddress + cm[i].endFileOffset - cm[i].startFileOffset) ) {
          return cartridgeBytes[addr - cm[i].mappedAddress + cm[i].startFileOffset]
        }
      }
      else if (cm[i].type=='ram') {
        if (isBetweenInclusive(addr, cm[i].mappedReadAddress, cm[i].mappedAddress+cm[i].length-1)) {
          return cm[i].memory[addr - cm[i].mappedAddress]
        }
      }
      else {
        return unreachable()
      }
    }



  }
  else {
    if (addr & 0x0080) {
      // PIA
      if (addr & 0x0200) {
        // PIA registers
        let regNo = addr & 0x0007
        if (regNo==0) {
          // SWCHA
          let swcha = [0,0,0,0,0,0,0,0] // will need reversing when serialization
          if (controllerType == 'joystick' || controllerType == 'joystick3') {
            swcha[0] = !(pressedKeys['KeyI'] && swacnt[0]==0 )
            swcha[1] = !(pressedKeys['KeyK'] && swacnt[1]==0 )
            swcha[2] = !(pressedKeys['KeyJ'] && swacnt[2]==0 )
            swcha[3] = !(pressedKeys['KeyL'] && swacnt[3]==0 )
            swcha[4] = !(pressedKeys['KeyW'] && swacnt[4]==0 )
            swcha[5] = !(pressedKeys['KeyS'] && swacnt[5]==0 )
            swcha[6] = !(pressedKeys['KeyA'] && swacnt[6]==0 )
            swcha[7] = !(pressedKeys['KeyD'] && swacnt[7]==0 )
            return parseInt(swcha.reverse().join(''))
          }
          else if (controllerType == 'paddle') {
            //// TODO
            return 0
          }
          else if (controllerType == 'keypad') {
            //// TODO
            return 0
          }
          else {
            unreachable()
          }
        }
        else if (regNo==1) {
          // SWACNT
          return parseInt(swacnt.reverse().join(' '))
        }
        else if (regNo==2) {
          // SWCHB
        }
        else if (regNo==3) {
          // SWBCNT
          return parseInt(swbcnt.reverse().join(' '))
        }
        else if (regNo==4 || regNo==6) {
          // INTIM
          return timer.value
        }
        else if (regNo==5 || regNo==7) {
          // INSTAT
          let v = 0
          if (timer.underflow) v += 128+64
          if (triggerIO) {
            timer.underflow = false
          }
          return v
        }
      }
      else {
        // PIA RAM
        return ram[addr & 0x007F]
      }
    }
    else {
      // TIA
      let regNo = addr & 0x000F
      if (regNo==0) {
        // CXM0P
      }
      else if (regNo==1) {
        // CXM1P
      }
      else if (regNo==2) {
        // CXP0FB
      }
      else if (regNo==3) {
        // CXP1FB
      }
      else if (regNo==4) {
        // CXM0FB
      }
      else if (regNo==5) {
        // CXM1FB
      }
      else if (regNo==6) {
        // CXBLPF
      }
      else if (regNo==7) {
        // CXPPMM
      }
      else if (regNo==8) {
        // INPT0
        if (controllerType=='joystick' || controllerType=='joystick3') {

        }
        else {
          return 0 // TODO
        }
      }
      else if (regNo==9) {
        // INPT1
      }
      else if (regNo==10) {
        // INPT2
      }
      else if (regNo==11) {
        // INPT3
      }
      else if (regNo==12) {
        // INPT4
      }
      else if (regNo==13) {
        // INPT5
      }
      else return readAddrNotMapped(addr)
    }
  }
}

function readAddrNotMapped(addr) {
  return 0 // TODO or garbage?
}

function setByteToMemory(addr, value) {
  if (addr & 0x1000) {
    // cartridge
    let addr = addr & 0xFFF
    let cm = cartridgeMapping
    for (let i = 0; i<cm.length; i++) {
      if (cm[i].triggerAddress == addr && (cm[i].triggerValue==value || cm[i].triggerValue==-1)) {

          cm[i].isActive = true
          
          for (let j = 0; j<cm.length; j++) {
            if (j==i) continue
            if (!cm[j].isActive) continue
  
            let a = cm[i].startFileOffset // the new region
            let b = cm[i].endFileOffset
            let c = cm[j].startFileOffset // the old region
            let d = cm[j].endFileOffset
  
            if ( !(b < c || d < a) ) {
              // they intersect
              cm[j].isActive = false
            }
          }

        


        return 0 // we assume there's only one triggerAddress for each entry
      }
    }

    // no triggerAddress is targetted. So we do the normal operation
    for (let i = 0; i<cm.length; i++) {
      if (!cm[i].isActive) continue

      if (cm[i].type=='rom') {
        /*
        301-300+1 = 2
        end-start+1

        madr <= adr <= madr+len-1
        0        0,1    len=2

        400 madr
        5 len
        adr: 400,401,402,403,404
          madr <= adr <= madr+len-1
        */
        if ( isBetweenInclusive(addr, cm[i].mappedAddress, cm[i].mappedAddress + cm[i].endFileOffset - cm[i].startFileOffset) ) {
          cartridgeBytes[addr - cm[i].mappedAddress + cm[i].startFileOffset] = value
        }
      }
      else if (cm[i].type=='ram') {
        if (isBetweenInclusive(addr, cm[i].mappedReadAddress, cm[i].mappedAddress+cm[i].length-1)) {
          cm[i].memory[addr - cm[i].mappedAddress] = value
        }
      }
      else {
        return unreachable()
      }
    }

  }
  else {
    if (addr & 0x0080) {
      // PIA
      if (addr & 0x0200) {
        // PIA registers
        let regNo = addr & 0x0007
        if (regNo==0) {
          // SWCHA
        }
        else if (regNo==1) {
          // SWACNT
          swacnt = value.toString(2).padStart(8,'0').split('').map(Number).reverse()
        }
        else if (regNo==2) {
          // SWCHB
        }
        else if (regNo==3) {
          // SWBCNT
          swbcnt = value.toString(2).padStart(8,'0').split('').map(Number).reverse()
        }
        else if (regNo==4) {
          // TIM1T
          if (value>0) {
            timer.underflow = false
            timer.value = value-1
          }
          else {
            timer.underflow = true
            timer.value = 255
          }
          timer.interval = 1
        }
        else if (regNo==5) {
          // TIM8T
          if (value>0) {
            timer.underflow = false
            timer.value = value-1
          }
          else {
            timer.underflow = true
            timer.value = 255
          }
          timer.interval = 8
        }
        else if (regNo==6) {
          // TIM64T
          if (value>0) {
            timer.underflow = false
            timer.value = value-1
          }
          else {
            timer.underflow = true
            timer.value = 255
          }
          timer.interval = 64
        }
        else if (regNo==7) {
          // T1024T
          if (value>0) {
            timer.underflow = false
            timer.value = value-1
          }
          else {
            timer.underflow = true
            timer.value = 255
          }
          timer.interval = 1024
        }
      }
      else {
        // PIA RAM
        ram[addr & 0x007F] = value
      }
    }
    else {
      // TIA
      let regNo = addr & 0x003F
      if (regNo==0x00) {
        // VSYNC
        let newVsync = (value & 2) >> 1
        if (vsync==1 && newVsync==0) {
          tvCtx.clearRect(0,0,canvasWidth,canvasHeight)
        }
        vsync = newVsync
      }
      else if (regNo==0x01) {
        // VBLANK
        vblank = (value & 2) >> 1
        // TODO vblank is used for joysticks
        vblankInpt[0] = (value >> 6) & 1
        vblankInpt[1] = (value >> 7) & 1
      }
      else if (regNo==0x02) {
        // WSYNC
        isWaitingHsync = true
      }
      else if (regNo==0x03) {
        // RSYNC
      }
      else if (regNo==0x04) {
        // NUSIZ0
        missile0.size = 2**( (value>>4)&3 )
        player0.numberSizeSetting = value & 0x7
      }
      else if (regNo==0x05) {
        // NUSIZ1
        missile1.size = 2**( (value>>4)&3 )
        player1.numberSizeSetting = value & 0x7
      }
      else if (regNo==0x06) {
        // COLUP0
        player0.color = (value >> 4) & 0xF
        player0.luminance = (value >> 1) & 0x7
      }
      else if (regNo==0x07) {
        // COLUP1
        player1.color = (value >> 4) & 0xF
        player1.luminance = (value >> 1) & 0x7
      }
      else if (regNo==0x08) {
        // COLUPF
        playfield.color = (value >> 4) & 0xF
        playfield.luminance = (value >> 1) & 0x7
      }
      else if (regNo==0x09) {
        // COLUBK
        background.color = (value >> 4) & 0xF
        background.luminance = (value >> 1) & 0x7
      }
      else if (regNo==0x0A) {
        // CTRLPF
        playfield.isReflected = value & 1
        isScoreMode = (value >> 1) & 1
        isBallPriorityMode = (value >> 2) & 1
        ball.size = 2**( (value>>4)&3 )
      }
      else if (regNo==0x0B) {
        // REFP0
        player0.isReflected = (value >> 3) & 1
      }
      else if (regNo==0x0C) {
        // REFP1
        player1.isReflected = (value >> 3) & 1
      }
      else if (regNo==0x0D) {
        // PF0
        let v = value.toString(2).padStart(8,'0').split('').map(Number)
        playfield.sprite[0] = v[3]
        playfield.sprite[1] = v[2]
        playfield.sprite[2] = v[1]
        playfield.sprite[3] = v[0]
      }
      else if (regNo==0x0E) {
        // PF1
        let v = value.toString(2).padStart(8,'0').split('').map(Number)
        for (let i = 0; i<8; i++) {
          playfield.sprite[4+i] = v[i]
        }
      }
      else if (regNo==0x0F) {
        // PF2
        let v = value.toString(2).padStart(8,'0').split('').map(Number)
        for (let i = 0; i<8; i++) {
          playfield.sprite[12+i] = v[7-i]
        }
      }
      else if (regNo==0x10) {
        // RESP0
      }
      else if (regNo==0x11) {
        // RESP1
      }
      else if (regNo==0x12) {
        // RESM0
      }
      else if (regNo==0x13) {
        // RESM1
      }
      else if (regNo==0x14) {
        // RESBL
      }
      else if (regNo==0x15) {
        // AUDC0
      }
      else if (regNo==0x16) {
        // AUDC1
      }
      else if (regNo==0x17) {
        // AUDF0
      }
      else if (regNo==0x18) {
        // AUDF1
      }
      else if (regNo==0x19) {
        // AUDV0
      }
      else if (regNo==0x1A) {
        // AUDV1
      }
      else if (regNo==0x1B) {
        // GRP0
        player0.sprite = value.toString(2).padStart(8,'0').split('').map(Number)
      }
      else if (regNo==0x1C) {
        // GRP1
        player1.sprite = value.toString(2).padStart(8,'0').split('').map(Number)
      }
      else if (regNo==0x1D) {
        // ENAM0
        missile0.isVisible = (value >> 1) & 1
      }
      else if (regNo==0x1E) {
        // ENAM1
        missile1.isVisible = (value >> 1) & 1
      }
      else if (regNo==0x1F) {
        // ENABL
        ball.isVisible = (value >> 1) & 1
      }
      else if (regNo==0x20) {
        // HMP0
        let v = (value >> 4) & 0xF
        if (v > 7) v = (v & 7) - 8
        else v = v & 7
        player0.positionAdjustment = v
      }
      else if (regNo==0x21) {
        // HMP1
        let v = (value >> 4) & 0xF
        if (v > 7) v = (v & 7) - 8
        else v = v & 7
        player1.positionAdjustment = v
      }
      else if (regNo==0x22) {
        // HMM0
        let v = (value >> 4) & 0xF
        if (v > 7) v = (v & 7) - 8
        else v = v & 7
        missile0.positionAdjustment = v
      }
      else if (regNo==0x23) {
        // HMM1
        let v = (value >> 4) & 0xF
        if (v > 7) v = (v & 7) - 8
        else v = v & 7
        missile1.positionAdjustment = v
      }
      else if (regNo==0x24) {
        // HMBL
        let v = (value >> 4) & 0xF
        if (v > 7) v = (v & 7) - 8
        else v = v & 7
        ball.positionAdjustment = v
      }
      else if (regNo==0x25) {
        // VDELP0
      }
      else if (regNo==0x26) {
        // VDELP1
      }
      else if (regNo==0x27) {
        // VDELBL
      }
      else if (regNo==0x28) {
        // RESMP0
        missile0.isPositionLockedOnPlayer = (value >> 1) & 1
      }
      else if (regNo==0x29) {
        // RESMP1
        missile1.isPositionLockedOnPlayer = (value >> 1) & 1
      }
      else if (regNo==0x2A) {
        // HMOVE
      }
      else if (regNo==0x2B) {
        // HMCLR
      }
      else if (regNo==0x2C) {
        // CXCLR
      }
      else writeAddrNotMapped(addr, value)
    }
  }
}

function writeAddrNotMapped(addr, value) {
  // nop
}

function loadValueUsingInst(inst) {
  if (inst.addressingMode=='Implied') {
    return regA
  }
  else if (inst.addressingMode=='Immediate') {
    return inst.operand
  }
  else if (inst.addressingMode=='Immediate2') {
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
  else if (inst.addressingMode=='Absolute2') {
    return getByteFromMemory(inst.operand, true) | getByteFromMemory(inst.operand+1, true)<<8
  }
  else if (inst.addressingMode=='AbsoluteX') {
    if (crossesPageBoundary(inst.operand, regX)) {
      getByteFromMemory(inst.operand+regX-256, true)
    }
    return getByteFromMemory(inst.operand+regX, true)
  }
  else if (inst.addressingMode=='AbsoluteY') {
    if (crossesPageBoundary(inst.operand, regY)) {
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
    if (crossesPageBoundary(addr, regY)) {
      getByteFromMemory(addr+regY-256, true)
    }
    return getByteFromMemory(addr+regY, true)
  }
  else {
    unreachable()
  }
}

function storeValueUsingInst(inst, value) {
  if (inst.addressingMode=='Implied') {
    regA = value
  }
  else if (inst.addressingMode=='Immediate') {
    unreachable()
  }
  else if (inst.addressingMode=='ZeroPage') {
    setByteToMemory(inst.operand, value)
  }
  else if (inst.addressingMode=='ZeroPageX') {
    setByteToMemory((inst.operand+regX)%256, value)
  }
  else if (inst.addressingMode=='ZeroPageY') {
    setByteToMemory((inst.operand+regY)%256, value)
  }
  else if (inst.addressingMode=='Absolute') {
    setByteToMemory(inst.operand, value)
  }
  else if (inst.addressingMode=='AbsoluteX') {
    setByteToMemory(inst.operand+regX, value)
  }
  else if (inst.addressingMode=='AbsoluteY') {
    setByteToMemory(inst.operand+regY, value)
  }
  else if (inst.addressingMode=='IndirectX') {
    let addr = getByteFromMemory((inst.operand+regX)%256, false) + // TODO: are these true or false?
      getByteFromMemory((inst.operand+regX)%256+1, false) << 8
    setByteToMemory(addr, value)
  }
  else if (inst.addressingMode=='IndirectY') {
    let addr = getByteFromMemory(inst.operand, false) +
      getByteFromMemory(inst.operand+1, false) << 8
    setByteToMemory(addr+regY, value)
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

function updateNZ(val) {
  flagZ = val==0
  flagN = val>127
}

function isBetweenInclusive(a, b, c) { // is a between b and c? both sides inclusive
  return a >= b && a <= c
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
    {firstByte: 0x6C,	opcode: 'JMP',	addressingMode: 'Absolute2',	cycles: 5},
    {firstByte: 0x20,	opcode: 'JSR',	addressingMode: 'Immediate2',	cycles: 6},
    {firstByte: 0x40,	opcode: 'RTI',	addressingMode: 'Implied',	cycles: 6},
    {firstByte: 0x60,	opcode: 'RTS',	addressingMode: 'Implied',	cycles: 6},
    {firstByte: 0x10,	opcode: 'BPL',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0x30,	opcode: 'BMI',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0x50,	opcode: 'BVC',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0x70,	opcode: 'BVS',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0x90,	opcode: 'BCC',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0xB0,	opcode: 'BCS',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0xD0,	opcode: 'BNE',	addressingMode: 'PCRelative',	cycles: 2},
    {firstByte: 0xF0,	opcode: 'BEQ',	addressingMode: 'PCRelative',	cycles: 2},
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
    Absolute2: 2,
    AbsoluteX: 2,
    AbsoluteY: 2,
    IndirectX: 1,
    IndirectY: 1,
    PCRelative: 1
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
    if (inst.addressingMode=='PCRelative') {
      currentInst.operand -= 128
    }
    // TODO: if pc is FFFF then this should return to 0 or what?
    regPC+=2
  }
  else if (addressingModeToOperandLength[inst.addressingMode]==2) {
    currentInst.operand = getByteFromMemory(regPC+1, true) | getByteFromMemory(regPC+2, true)<<8
    regPC+=3
  }
  else {
    unreachable()
  }

  executeAfter = inst.cycles
  if (!['STA','STX','STY','INC','INX','INY','DEC','DEX','DEY','ASL','LSR','ROL','ROR'].includes(inst.opcode)) {
    if (inst.addressingMode=='AbsoluteX') {
      if (crossesPageBoundary(currentInst.operand, regX)) executeAfter++
    }
    else if (inst.addressingMode=='AbsoluteY') {
      if (crossesPageBoundary(currentInst.operand, regY)) executeAfter++
    }
    else if (inst.addressingMode=='IndirectY') {
      let val = getByteFromMemory(currentInst.operand, false) | getByteFromMemory(currentInst.operand+1, false)<<8
      if (crossesPageBoundary(val, regY)) executeAfter++
    }
    else if (inst.addressingMode=='PCRelative') {
      if (!shouldBranch(inst.opcode)) {}
      else if (!crossesPageBoundary(regPC, currentInst.operand)) executeAfter++
      else executeAfter+=2
    }
  }
}

function crossesPageBoundary(arg1, arg2) {
  let sum = arg1%256 + arg2%256
  return sum>=256 || sum<0
}

function shouldBranch(opcode) {
  if (opcode=='BPL') return flagN==0
  else if (opcode=='BMI') return flagN==1
  else if (opcode=='BVC') return flagV==0
  else if (opcode=='BVS') return flagV==1
  else if (opcode=='BCC') return flagC==0
  else if (opcode=='BCS') return flagC==1
  else if (opcode=='BNE') return flagZ==0
  else if (opcode=='BEQ') return flagZ==1
  else unreachable()
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

function newRomEntry(startFileOffset, endFileOffset, mappedAddress, triggerAddress, triggerValue, isActive) {
  return {
    type: 'rom',
    startFileOffset,
    endFileOffset,
    mappedAddress,
    triggerAddress,
    triggerValue,
    isActive
  }
}

function newRamEntry(length, mappedReadAddress, mappedWriteAddress) {
  return {
    type: 'ram',
    length,
    mappedReadAddress,
    mappedWriteAddress,
    isActive: true // ram is always active
  }
}

let recognizedCartridgeMappings = [
  // atari 4k
  [
    newRomEntry(0,        0xFFF,    0,        -1,       -1,       true)
  ],

  // atari 2k
  [
    newRomEntry(0,        0x7FF,    0,        -1,       -1,       true),
    newRomEntry(0,        0x7FF,    0x800,    -1,       -1,       true)
  ],

  // atari 8k
  [
    newRomEntry(0,        0xFFF,    0,        0xFF8,    -1,       true),
    newRomEntry(0x1000,   0x1FFF,   0,        0xFF9,    -1,       false),
  ],

  // atari 12k
  [
    newRomEntry(0,        0xFFF,    0,        0xFF8,    -1,       true),
    newRomEntry(0x1000,   0x1FFF,   0,        0xFF9,    -1,       false),
    newRomEntry(0x2000,   0x2FFF,   0,        0xFFA,    -1,       false),
  ],

  // atari 16k
  [
    newRomEntry(0,        0xFFF,    0,        0xFF6,    -1,       true),
    newRomEntry(0x1000,   0x1FFF,   0,        0xFF7,    -1,       false),
    newRomEntry(0x2000,   0x2FFF,   0,        0xFF8,    -1,       false),
    newRomEntry(0x3000,   0x3FFF,   0,        0xFF9,    -1,       false),
  ],


  // atari 32k
  [
    newRomEntry(0,        0xFFF,    0,        0xFF4,    -1,       true),
    newRomEntry(0x1000,   0x1FFF,   0,        0xFF5,    -1,       false),
    newRomEntry(0x2000,   0x2FFF,   0,        0xFF6,    -1,       false),
    newRomEntry(0x3000,   0x3FFF,   0,        0xFF7,    -1,       false),
    newRomEntry(0x4000,   0x4FFF,   0,        0xFF8,    -1,       false),
    newRomEntry(0x5000,   0x5FFF,   0,        0xFF9,    -1,       false),
    newRomEntry(0x6000,   0x6FFF,   0,        0xFFA,    -1,       false),
    newRomEntry(0x7000,   0x7FFF,   0,        0xFFB,    -1,       false),
  ],


  // burgtime, he_man
  [
    newRomEntry(0,        0x7FF,    0,        0xFE0,    -1,       true),
    newRomEntry(0x800,    0xFFF,    0,        0xFE1,    -1,       false),
    newRomEntry(0x1000,   0x17FF,   0,        0xFE2,    -1,       false),
    newRomEntry(0x1800,   0x1FFF,   0,        0xFE3,    -1,       false),
    newRomEntry(0x2000,   0x27FF,   0,        0xFE4,    -1,       false),
    newRomEntry(0x2800,   0x2FFF,   0,        0xFE5,    -1,       false),
    newRomEntry(0x3000,   0x37FF,   0,        0xFE6,    -1,       false),
    newRomEntry(0x3800,   0x3FFF,   0,        0xFE7,    -1,       false),
    newRomEntry(0x3A00,   0x3FFF,   0xA00,    -1,       -1,       true),  // TODO: 3a00-3fff or 3800-3dff???
    newRamEntry(0x200,    0x800,    0x800)
  ],

  // TODO: analyze BNJ rom file. what does "first 8k empty" mean?


  // Popeye, Gyruss, Sprcobra, Tutank, Dethstar, Qbrtcube, Ewokadvn, Montzrev, Frogger2, Lordofth, James_bo, Swarcade, Toothpro
  [
    newRomEntry(0,        0x3FF,    0,        0xFE0,     -1,       true),
    newRomEntry(0x400,    0x7FF,    0,        0xFE1,     -1,       false),
    newRomEntry(0x800,    0xBFF,    0,        0xFE2,     -1,       false),
    newRomEntry(0xC00,    0xFFF,    0,        0xFE3,     -1,       false),
    newRomEntry(0x1000,   0x13FF,   0,        0xFE4,     -1,       false),
    newRomEntry(0x1400,   0x17FF,   0,        0xFE5,     -1,       false),
    newRomEntry(0x1800,   0x1BFF,   0,        0xFE6,     -1,       false),
    newRomEntry(0x1C00,   0x1FFF,   0,        0xFE7,     -1,       false),
    newRomEntry(0,        0x3FF,    0x400,    0xFE8,     -1,       true),
    newRomEntry(0x400,    0x7FF,    0x400,    0xFE9,     -1,       false),
    newRomEntry(0x800,    0xBFF,    0x400,    0xFEA,     -1,       false),
    newRomEntry(0xC00,    0xFFF,    0x400,    0xFEB,     -1,       false),
    newRomEntry(0x1000,   0x13FF,   0x400,    0xFEC,     -1,       false),
    newRomEntry(0x1400,   0x17FF,   0x400,    0xFED,     -1,       false),
    newRomEntry(0x1800,   0x1BFF,   0x400,    0xFEE,     -1,       false),
    newRomEntry(0x1C00,   0x1FFF,   0x400,    0xFEF,     -1,       false),
    newRomEntry(0,        0x3FF,    0x800,    0xFF0,     -1,       true),
    newRomEntry(0x400,    0x7FF,    0x800,    0xFF1,     -1,       false),
    newRomEntry(0x800,    0xBFF,    0x800,    0xFF2,     -1,       false),
    newRomEntry(0xC00,    0xFFF,    0x800,    0xFF3,     -1,       false),
    newRomEntry(0x1000,   0x13FF,   0x800,    0xFF4,     -1,       false),
    newRomEntry(0x1400,   0x17FF,   0x800,    0xFF5,     -1,       false),
    newRomEntry(0x1800,   0x1BFF,   0x800,    0xFF6,     -1,       false),
    newRomEntry(0x1C00,   0x1FFF,   0x800,    0xFF7,     -1,       false),
    newRomEntry(0x1C00,   0x1FFF,   0xC00,    -1,        -1,       true)
  ]

  // TODO: Cart 2k-banking not implemented because of 3F addr issue (Springer, Espial, Minrvol2, Mnr2049r, Riverp, Polaris)
  
  // Decathln and Pitfall 2 not supported
]


//  ctx.fillStyle = `hsv(${hsv[0]}, ${hsv[1]}, ${hsv[2]})`;
// where tvStandard = NTSC, PAL, SECAM, MONO
function getColorHSV(tvStandard, colu) {
  let bits123 = (colu >> 1) & 7
  let bits4567 = (colu >> 4) & 15

  if (tvStandard == 'ntsc') {
    let ntscColorsHSV = [
      [0, 0, 1],     // White
      [50/360, 1, 1],  // Gold
      [30/360, 1, 1],  // Orange
      [30/360, 1, 1],  // Bright orange
      [330/360, 1, 1],  // Pink
      [300/360, 1, 1],  // Purple
      [270/360, 1, 1],  // Purple-blue
      [240/360, 1, 1],  // Blue
      [240/360, 1, 1],  // Blue
      [210/360, 1, 1],  // Light blue
      [180/360, 1, 1],  // Torq.
      [150/360, 1, 1],  // Green-blue
      [120/360, 1, 1],  // Green
      [90/360, 1, 1],   // Yellow-green
      [60/360, 1, 1],   // Orange-green
      [30/360, 1, 1]    // Light orange
    ]
    let color = ntscColorsHSV[bits4567]
    color[2] = color[2] * bits123 / 7
    return color
  }  
  else if (tvStandard == 'pal') {
    let palColorsHSV = [
      [0, 0, 1],      // White
      [0, 0, 1],      // White
      [60/360, 1, 1],   // Yellow
      [90/360, 1, 1],   // Green-yellow
      [30/360, 1, 1],   // Orange
      [120/360, 1, 1],  // Green
      [0/360, 1, 1],    // Pink-orange
      [150/360, 1, 1],  // Green-blue
      [330/360, 1, 1],  // Pink
      [180/360, 1, 1],  // Turquois
      [300/360, 1, 1],  // Pink-blue
      [210/360, 1, 1],  // Light blue
      [240/360, 1, 1],  // Blue-red
      [240/360, 1, 1],  // Dark blue
      [0, 0, 1],      // Same as color 0
      [0, 0, 1]       // Same as color 0
    ]
    let color = palColorsHSV[bits4567]
    color[2] = color[2] * bits123 / 7
    return color
  }  
  else if (tvStandard == 'secam') {
    let secamColorsHSV = [
      [0, 0, 0],        // Black
      [240/360, 1, 1],  // Blue
      [0/360, 1, 1],    // Red
      [300/360, 1, 1],  // Magenta
      [120/360, 1, 1],  // Green
      [180/360, 1, 1],  // Cyan
      [60/360, 1, 1],   // Yellow
      [0, 0, 1]         // White
    ]
    return secamColorsHSV[bits123]
  }  
  else { // Mono color TV
    let grayshadeColorsHSV = [
      [0, 0, 0],      // Black
      [0, 0, 0.25],   // Dark grey
      [0, 0, 0.5],    // Grey
      [0, 0, 0.75],   // Light grey
      [0, 0, 0.875],  // Greyish white
      [0, 0, 0.94],   // Lightest grey
      [0, 0, 0.97],   // Slightly off-white
      [0, 0, 1]       // White
    ]
    return grayshadeColorsHSV[bits123]
  }  
}  

