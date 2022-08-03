//# RUN: peony %s

fn foo() -> f32 { return 0.0; }
fn bar() -> i32 { return 0; }

fn f() {
    5 + 8;
    2 + 3 * 7;
    1.0 / 7.0;
    3.0 - foo();

    true && false;
    true || false;

    5 && 2.0;
    //~^ ERROR: mismatched types; expected 'bool', found 'i32'
    //~| ERROR: mismatched types; expected 'bool', found 'f32'
    5 || 2.0;
    //~^ ERROR: mismatched types; expected 'bool', found 'i32'
    //~| ERROR: mismatched types; expected 'bool', found 'f32'

    if 5 == 1 {}
    if 8.0 >= 3.0 {}

    if 5 == 1.0 {}
    //~^ ERROR: cannot apply binary operator '==' to types 'i32' and 'f32'
    if 5 != 1.0 {}
    //~^ ERROR: cannot apply binary operator '!=' to types 'i32' and 'f32'
    if 5 < 1.0 {}
    //~^ ERROR: cannot apply binary operator '<' to types 'i32' and 'f32'
    if 5 <= 1.0 {}
    //~^ ERROR: cannot apply binary operator '<=' to types 'i32' and 'f32'
    if 5 > 1.0 {}
    //~^ ERROR: cannot apply binary operator '>' to types 'i32' and 'f32'
    if 5 >= 1.0 {}
    //~^ ERROR: cannot apply binary operator '>=' to types 'i32' and 'f32'

    5 & 8;
    5 ^ 8;
    5 | 8;
    5 << 8;
    5 >> 8;
    let a0 = 5;
    a0 &= 8;
    a0 ^= 8;
    a0 |= 8;
    a0 <<= 8;
    a0 >>= 8;

    true & 9.0;
    //~^ ERROR: mismatched types; expected '{integer}', found 'bool'
    //~| ERROR: mismatched types; expected '{integer}', found 'f32'
    true ^ 9.0;
    //~^ ERROR: mismatched types; expected '{integer}', found 'bool'
    //~| ERROR: mismatched types; expected '{integer}', found 'f32'
    true | 9.0;
    //~^ ERROR: mismatched types; expected '{integer}', found 'bool'
    //~| ERROR: mismatched types; expected '{integer}', found 'f32'
    true << 9.0;
    //~^ ERROR: mismatched types; expected '{integer}', found 'bool'
    //~| ERROR: mismatched types; expected '{integer}', found 'f32'
    true >> 9.0;
    //~^ ERROR: mismatched types; expected '{integer}', found 'bool'
    //~| ERROR: mismatched types; expected '{integer}', found 'f32'

    a0 ^= 0.0;
    //~^ ERROR: mismatched types; expected 'i32', found 'f32'

    let a1 = 0.0;
    a1 &= 5;
    //~^ ERROR: binary assignment operation '&=' cannot be applied to type 'f32'
    a1 ^= 5;
    //~^ ERROR: binary assignment operation '^=' cannot be applied to type 'f32'
    a1 |= 5;
    //~^ ERROR: binary assignment operation '|=' cannot be applied to type 'f32'
    a1 >>= 5;
    //~^ ERROR: binary assignment operation '>>=' cannot be applied to type 'f32'
    a1 <<= 5;
    //~^ ERROR: binary assignment operation '<<=' cannot be applied to type 'f32'

    // Special messages for classic operators.
    5 + true;
    //~^ ERROR: cannot add 'i32' to 'bool'
    a1 += true;
    //~^ ERROR: cannot add-assign 'bool' to 'f32'
    5u32 - 2f64;
    //~^ ERROR: cannot subtract 'f64' from 'u32'
    a1 -= true;
    //~^ ERROR: cannot subtract-assign 'bool' from 'f32'
    8.0f32 * 16f64;
    //~^ ERROR: cannot multiply 'f32' by 'f64'
    a1 *= true;
    //~^ ERROR: cannot multiply-assign 'f32' by 'bool'
    true / 5;
    //~^ ERROR: cannot divide 'bool' by 'i32'
    a1 /= true;
    //~^ ERROR: cannot divide-assign 'f32' by 'bool'
}
