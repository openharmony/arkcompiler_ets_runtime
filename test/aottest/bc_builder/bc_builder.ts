declare function print(arg: any): void;

function testDeadBlock() {
    let a: number = 0;
    try {
        a = 2;
    } catch(error) {
        a = 1;
    }
}

testDeadBlock();

function testLoop(obj) {
    while(obj.next) {
        obj = obj.next        
    }
}

testLoop({next: {}});

function testLoop1(opt) {
    var current = opt || null;
    while (current.next) {
      current = current.next;
    }
    return current;
}
testLoop2({next: {}});

function testLoop2() {
    let a = 0;

    for (let i = 0; i < 10; i++) {
        for (let j = 0; j < 10; j++) {
            a++;
        }
    }
    return a;
}
testLoop2();

function testLoop3(opt) {
    for (let i = 0; i < 10; i++) {
        if (i == 0) {
            var current = opt || null;
            while (current.next) {
                current = current.next;
            }
            break;
        }
    }
    return current;
}

testLoop3({next: {}});

function testEmptyIf(f: boolean) {
    let a = 0;
    if (f) {

    }
    a++;
}

testEmptyIf();

function testWhileIf(f: boolean) {
    let a = 0;
    while(a < 10) {
        if (f) {
        } else {
            a++;
        }
    }
}
testWhileIf(false);

function testWhileIf1(f: boolean) {
    let a = 0;
    if (f) {
        a++;
    }
    while(a < 10) {
        a++;
    }
}
testWhileIf1(false);
print(2);
