# RUN: peony -fsyntax-only -verify %s

fn foo(x: i32) {}
fn bar() { foo(); } # EXPECT-ERROR: too few arguments to function 'fn foo(i32) -> void'
