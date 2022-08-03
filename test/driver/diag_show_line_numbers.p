//# RUN: peony -fdiagnostics-show-line-numbers %s
//# RUN: peony -fno-diagnostics-show-line-numbers %s

fn test() {
    5 + true;
    //~^ ERROR: cannot add 'i32' to 'bool'
}
