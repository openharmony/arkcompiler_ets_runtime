let x = 0
let y = 3.14
let z = x + y
ArkTools.pgoAssertType(z, "number"); // int + int -> int

y = 0
z = x + y
ArkTools.pgoAssertType(z, "int");

x = 3.14
y = 3.14
z = x + y
ArkTools.pgoAssertType(z, "double");

class A {
    constructor(a, b) {
        this.a = 1
        this.b = 2.2
    }
}

let instanceA = new A();

let resultA = instanceA.a + 1;
ArkTools.pgoAssertType(resultA, "int");

let resultB = instanceA.b + 3.2;
ArkTools.pgoAssertType(resultB, "double");
