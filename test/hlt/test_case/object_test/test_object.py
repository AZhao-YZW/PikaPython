# Object support
print("=====Test Start=====")
# Class Definition
class Animal:
    def __init__(self, name):
        self.name = name
    
    def speak(self):
        return "%s makes a sound" %(self.name)

# Class Inheritance
class Dog(Animal):
    def speak(self):
        return "%s barks" %(self.name)

# Method Definition
class Bird(Animal):
    def fly(self):
        return "%s is flying" %(self.name)

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
        print("%s %s!" %(greeting, name))

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

count = 0
while count < 3:
    print("Count is %s" %(count))
    count += 1

print("=====Test End=====")