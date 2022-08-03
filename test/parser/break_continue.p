//# RUN: peony %s

fn test() {
    break;
    //~^ ERROR: 'break' outside of a loop
    continue;
    //~^ ERROR: 'continue' outside of a loop

    {
        break;
        //~^ ERROR: 'break' outside of a loop
        continue;
        //~^ ERROR: 'continue' outside of a loop
    }

    if true {
        break;
        //~^ ERROR: 'break' outside of a loop
        continue;
        //~^ ERROR: 'continue' outside of a loop
    }

    while true {
        break;
        continue;
    }

    loop {
        break;
        continue;
    }
}
