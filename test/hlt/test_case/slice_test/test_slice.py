def test_slice(strings, byte, numbers):
    
    str1 = strings[7]
    print("str1:", str1)
    str2 = strings[7:17]
    print("str2:", str2)
    str3 = strings[7:]
    print("str3:", str3)

    byte1 = byte[7]
    print("byte1:", chr(byte1))
    byte2 = byte[7:17]
    print("byte2:", byte2.decode())
    byte3 = byte[7:]
    print("byte3:", byte3.decode())

    list1 = numbers[3]
    print("list1:", list1)
    list2 = numbers[2:5]
    print("list2:", list2)
    list3 = numbers[4:]
    print("list3:", list3)

strings = "Hello, PikaPython!"
byte = b"Hello, PikaPython!"
numbers = [1, 2, 3, 4, 5, 6, 7]
test_slice(strings, byte, numbers)
