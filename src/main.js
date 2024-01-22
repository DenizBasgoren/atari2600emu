
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
