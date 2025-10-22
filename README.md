Hello World

Overview:
- A minimal Python script that prints "hello world" and exposes a hello() function returning the same string.

Files:
- hello.py: Contains hello() and a CLI entrypoint that prints the message.
- tests/test_hello.py: Pytest verifying both function return value and script output.

Requirements:
- Python 3.8+ (tested with Python 3.x in this environment)
- pytest (for running tests)

Usage:
- Run the script: python3 hello.py
  Expected output: hello world

- Use as a module:
  from hello import hello
  print(hello())  # -> "hello world"

Testing:
- Run: pytest -q
