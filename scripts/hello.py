greeting = message.message.strip().capitalize()
if greeting == "":
    greeting = "Hello"
print greeting+" "+message.user.name+"!"
