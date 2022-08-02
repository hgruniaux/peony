# RUN: peony -fsyntax-only -verify %s

fn f() {
    let f0 = 0.0_;
    let f1 = 5_f32;
    let f2 = 254f64;
    let f3 = 8_2e+_986_;
    let f4 = 2.5e+3;
    let f5 = 32_.2_3e+23f64;
}
