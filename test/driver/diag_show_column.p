//# RUN: peony -fdiagnostics-show-column %s
//# RUN: peony -fno-diagnostics-show-column %s

fn test() {
    5 + true;
    //~^ ERROR: cannot add 'i32' to 'bool'
}
