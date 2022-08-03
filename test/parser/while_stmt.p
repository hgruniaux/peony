//# RUN: peony %s

fn test() {
    while true {}

    while 5i32 {}
    //~^ ERROR: mismatched types; expected 'bool', found 'i32'

    while () {}
    //~^ ERROR: expected expression
}
