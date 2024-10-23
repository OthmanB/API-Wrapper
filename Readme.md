# API Wrapper

This program allows you to encapsulate any other command-line program that does not have an API into a basic yet powerful API.

By default, it wraps the `stress-ng` program, but it can effectively wrap any program that runs using command-line arguments only.

## Dependencies

- **Crow**: A C++ microframework for web applications.
- **Boost**: The Boost library must be properly installed before compilation.
- **CMake**: For building the project.
- **G++**: A modern C++ compiler supporting the C++17 standard.
- **stress-ng**: The default program to be wrapped.

## Installation

### Install Dependencies

Ensure that the following dependencies are installed on your system:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ libboost-program-options-dev stress-ng
```

### Compile the Program

1. Create a build directory:
```bash 
mkdir build
cd build
```
2. Run CMake and compile:
```bash
cmake ..
make
```
3. Return to the root program directory:
```bash
cd ..
```
## Usage Instructions

### Running the Program
To run the program with default option, use the following command (not recommended the first time):
```bash
./build/wrapper_api
```

### Command-Line Options
You can check all available options by typing (recommeded before first usage):
```bash
./build/wrapper_api --help
```

### Example Usage
The wrapper can be used with commands accessible at the system level (path given in environment variables) or with a local program. The detection is automatic, but if you have two versions, you can use the --prefer-local flag to decide which takes precedence.

## Contributing
No contributino is expected. But feel free to submit issues or pull requests if you find any bugs or have suggestions for improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for details.