//# RUN: peony %s

fn f0() {}
fn f1(i: i32) -> i32 {}
fn f2(i: i32, j: i32) {}
fn f3(i: i32, ) {}
//~^ ERROR: expected parameter declarator

fn f3() {}
//~^ ERROR: redeclaration of function 'f3'

fn f4(x: i32, x: i32) {}
//~^ ERROR: identifier 'x' is bound more than once in this parameter list

fn f5(x: i32, y: i32 = 5) {}

fn f6(x: i32 = 5, y: i32) {}
//~^ ERROR: default argument missing for parameter 2 of function 'f6'

fn f7(,) {}
//~^ ERROR: expected parameter declarator
//~| ERROR: expected parameter declarator

fn f8()
//~^ ERROR: expected function body after function declarator
