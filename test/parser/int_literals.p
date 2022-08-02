# RUN: peony -fsyntax-only -verify %s

fn f() {
    let d0 = 0;
    let d1 = 105;
    let d2 = 1_0_5;
    let d3 = 1_05__;
    let d4 = 6484515111111684548645684858465644495555132795; # EXPECT-ERROR: integer literal too large

    let b0 = 0b0;
    let b1 = 0b10001;
    let b3 = 0B1_01;
    let b4 = 0B__101__;
    let b5 = 0B1111111111111111111111111111111111111111111111111111111111111111111111; # EXPECT-ERROR: integer literal too large

    let h0 = 0x15;
    let h1 = 0Xabcdef;
    let h3 = 0xABDC_EF_;
    let h4 = 0x_aAf_dEC__;
    let h5 = 0XffFFfFffffffffffffffffffF; # EXPECT-ERROR: integer literal too large
}
