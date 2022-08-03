//# RUN: peony %s

fn foo() {}
fn bar(a: i32) {}

fn test() {
    foo();
    bar(5);

    foo(5);
    //~^ ERROR: too many arguments to function 'fn foo() -> void'
    bar();
    //~^ ERROR: too few arguments to function 'fn bar(i32) -> void'

    let x = 5;
    x();
    //~^ ERROR: 'x' cannot be used as a function
    5();
    //~^ ERROR: expression cannot be used as a function

    bar(true);
    //~^ ERROR: mismatched types; expected 'i32', found 'bool'
}
