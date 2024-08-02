def test_operators(a, b):
    print("Addition (a + b):", a + b)
    print("Subtraction (a - b):", a - b)
    print("Multiplication (a * b):", a * b)
    print("Division (a / b):", a / b)
    print("Equal (a == b):", a == b)
    print("Greater than (a > b):", a > b)
    print("Less than (a < b):", a < b)
    print("Greater than or equal (a >= b):", a >= b)
    print("Less than or equal (a <= b):", a <= b)
    print("Modulo (a % b):", a % b)
    print("Exponent (a ** b):", a ** b)
    print("Floor division (a // b):", a // b)
    print("Not equal (a != b):", a != b)
    print("Bitwise AND (a & b):", a & b)
    print("Bitwise right shift (a >> b):", a >> b)
    print("Bitwise left shift (a << b):", a << b)
    print("Logical AND (a and b):", a and b)
    print("Logical OR (a or b):", a or b)
    print("Logical NOT (not a):", not a)
    print("In (a in [b]):", a in [b])
    a += b
    print("Augmented Addition (a += b):", a)
    a -= b
    print("Augmented Subtraction (a -= b):", a)
    a *= b
    print("Augmented Multiplication (a *= b):", a)
    a /= b
    print("Augmented Division (a /= b):", a)

a = 10
b = 5
test_operators(a, b)
