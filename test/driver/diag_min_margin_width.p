//# RUN: peony -fdiagnostics-minimum-margin-width=0 %s
//# RUN: peony -fdiagnostics-minimum-margin-width=3 %s
//# RUN: peony -fdiagnostics-minimum-margin-width=8 %s

fn test() {
    5 + true;
    //~^ ERROR: cannot add 'i32' to 'bool'
}
