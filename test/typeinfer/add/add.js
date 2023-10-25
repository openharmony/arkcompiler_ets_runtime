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