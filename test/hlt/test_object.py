# Object support
print("=====Test Start=====")
# Module Import
import math
import sys

# Class Definition
class Animal:
    def __init__(self, name):
        self.name = name
    
    def speak(self):
        return f"{self.name} makes a sound"

# Class Inheritance
class Dog(Animal):
    def speak(self):
        return f"{self.name} barks"

# Method Definition
class Bird(Animal):
    def fly(self):
        return f"{self.name} is flying"

# Method Overloading
class Math:
    def add(self, a, b, c=0):
        return a + b + c

# Method Call
dog = Dog("Buddy")
print(dog.speak())

# Parameter Definition
def greet(greeting="Hello", *names):
    for name in names:
        print(f"{greeting}, {name}!")

# Parameter Assignment
greet("Hi", "Alice", "Bob")

# Object Creation
cat = Animal("Whiskers")
print(cat.speak())

# Object Destruction
del cat

# Object Nesting
class Owner:
    def __init__(self, name, pet):
        self.name = name
        self.pet = pet

owner = Owner("Alice", dog)
print(owner.pet.speak())

# Control Flow
if owner.name == "Alice":
    print("Owner is Alice")
elif owner.name == "Bob":
    print("Owner is Bob")
else:
    print("Unknown owner")

for i in range(3):
    print(f"Iteration {i}")

count = 0
while count < 3:
    print(f"Count is {count}")
    count += 1

print("=====Test End=====")