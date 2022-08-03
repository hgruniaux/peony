//# RUN: peony %s

fn f() {
    let d0 = 0;
    let d1 = 105;
    let d2 = 1_0_5;
    let d3 = 1_05__;
    let d4 = 6484515111111684548645684858465644495555132795;
    //~^ ERROR: integer literal too large

    let b0 = 0b0;
    let b1 = 0b10001;
    let b3 = 0B1_01;
    let b4 = 0B__101__;
    let b5 = 0B1111111111111111111111111111111111111111111111111111111111111111111111;
    //~^ ERROR: integer literal too large

    let h0 = 0x15;
    let h1 = 0Xabcdef;
    let h3 = 0xABDC_EF_;
    let h4 = 0x_aAf_dEC__;
    let h5 = 0XffFFfFffffffffffffffffffF;
    //~^ ERROR: integer literal too large

    let c0 = 5i8;
    let c1 = 5i16;
    let c2 = 5i32;
    let c3 = 5i64;
    let c4 = 5u8;
    let c5 = 5u16;
    let c6 = 5u32;
    let c7 = 5u64;
}
