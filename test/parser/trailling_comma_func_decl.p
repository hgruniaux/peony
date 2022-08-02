// RUN: peony -fsyntax-only -verify %s

fn f0() {}
fn f1(i: i32) {}
fn f2(i: i32, j: i32) {}
fn f3(i: i32, ) {} # EXPECT-ERROR: expected parameter declarator
