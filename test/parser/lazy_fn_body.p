//# RUN: peony %s

// Function bodies are parsed lazily once all top-level declarations were parsed.
// Thus in function foo(), the call to bar() should not emit an error even if
// the function bar() was declared after foo().

fn foo() {
    let x = bar();
    x + 5;
}

fn bar() -> i32 {
}

// Check if the return type of f is correctly considered in the checks of the return statement.
fn f() -> f32 { return 0.0; }
