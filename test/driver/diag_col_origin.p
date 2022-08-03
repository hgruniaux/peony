//# RUN: peony -fdiagnostics-column-origin=0 %s
//# RUN: peony -fdiagnostics-column-origin=1 %s
//# RUN: peony -fdiagnostics-column-origin=6 %s

fn test() {
    5 + true;
    //~^ ERROR: cannot add 'i32' to 'bool'
}
