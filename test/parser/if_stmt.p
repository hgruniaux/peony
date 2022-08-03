//# RUN: peony %s

fn test() {
    if true {}
    if true {} else {}
    if true {} else if true {}
    if true {} else if true {} else {}

    if 5i32 {}
    //~^ ERROR: mismatched types; expected 'bool', found 'i32'

    if () {}
    //~^ ERROR: expected expression
}
