//# RUN: peony %s

fn foo() {}

fn r#i32() {
    let r#i32 = 5;
    if r#i32 == 8 {}
    r#foo();

    r#f();
    //~^ ERROR: use of undeclared identifier 'f'
}
