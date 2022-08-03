//# RUN: peony %s

fn f6(,
//~^ ERROR: expected parameter declarator
//~| ERROR: expected parameter declarator
//~| ERROR: expected ')', found 'EOF'
