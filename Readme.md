# API Wrapper

This program allows you to encapsulate any other command-line program that does not have an API into a basic yet powerful API.

By default, it wraps the `stress-ng` program, but it can effectively wrap any program that runs using command-line arguments only.

## Dependencies

- **Crow**: A C++ microframework for web applications.
- **Boost**: The Boost library must be properly installed before compilation.
- **CMake**: For building the project.
- **G++**: A modern C++ compiler supporting the C++17 standard.
- **stress-ng**: The default program to be wrapped.

## Local Build Instructions

### Install Dependencies

Ensure that the following dependencies are installed on your system. On a apt-based system such as ubuntu, it means:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ libboost-program-options-dev stress-ng
```

For a MacOS system, you may rely on ```brew``` to install those components,

```bash
brew update
brew install cmake boost stress-ng
```

Note that in all cases, you will need to update your environment variables to point towards your libboost installation.
For example, in MacOS:
```bash
export BOOST_ROOT=$(brew --prefix boost)
export BOOST_INCLUDEDIR=$BOOST_ROOT/include
export BOOST_LIBRARYDIR=$BOOST_ROOT/lib
export CPLUS_INCLUDE_PATH=$BOOST_INCLUDEDIR:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=$BOOST_LIBRARYDIR:$LIBRARY_PATH
export LD_LIBRARY_PATH=$BOOST_LIBRARYDIR:$LD_LIBRARY_PATH
```
Replace ```$(brew --prefix boost)``` by ```/usr/local/boost``` in a typical Linux-based system such as Ubuntu.

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

## Docker Build Instructions

### Building the Docker Image Locally (Architecture limited)

To build the Docker image for the local architecture, please first review the environment settings defining the option for the wrapper program within the ```Dockerfile```. The default is listening on port 8080 and all IP, with the child program being ```stress-ng```.

After review and setting, use the following command to build:

```bash
docker build -t yourusername/wrapper-docker .
```
Replace yourusername with your Docker Hub username or the appropriate registry path.

### Building the Docker Image for Multiple Architectures
Buildx supports various architectures. For example, to build the Docker image for both ARM and x86 architectures using Docker Buildx, follow these steps:
If it is not installed, you can check ```https://github.com/docker/buildx```.
1. Enable Docker Buildx:
```bash
docker buildx create --use
docker buildx inspect --bootstrap
```
2. Log in to Docker Hub (or your Docker registry):
```bash
docker login
```
3. Build and push the Docker image for multiple platforms:
```bash
docker buildx build --platform linux/amd64,linux/arm64 -t yourusername/wrapper-docker --push .
```
Replace ```yourusername``` with your Docker Hub username or the appropriate registry path.

## Usage Instructions

### Running the Program
For a local compilation, to run the program with default option, use the following command (not recommended the first time):
```bash
./build/wrapper_api
```

If you have a docker image use the command below, by specifying the option ```youroptions``` you want at the end for the wrapper,
```bash
docker run -p 8080:8080 yourusername/wrapper-docker youroptions
```
if you want to pass your own options to the program. Not all the options are shown, check the program help to get the full list.
Do not forget to replace ```yourusername``` by your account name in Docker Hub.

### Command-Line Options
You can check all available options by typing (recommeded before first usage):
```bash
./build/wrapper_api --help
```

### Example Usage
The wrapper can be used with commands accessible at the system level (path given in environment variables) or with a local program. The detection is automatic, but if you have two versions, you can use the --prefer-local flag to decide which takes precedence.

Two endpoints are available: ```/run``` and ```/results```. The ```run``` endpoint is for sending jobs. For example, to request the execution of the ```stress-ng``` (the default program, if not the option ```--job``` was not specified when executing the server with ```./wrapper_api```, it may look like this in curl:
```bash
curl -X POST http://localhost:8080/run -H "Content-Type: application/json" -d '{"flags": "--cpu 10 --timeout 20s -vm 4"}'
```
The -d options provided some flags necessary for ```stress-ng``` to run. The program is supposed to stress the machine for 20s. You may request result at anytime by using,
```bash
curl http://localhost:8080/results
```
But if you do so before the end of the execution of the program (within the 20s in the example), then you will receive a message specifying that the results are not ready. If you do so after, the result (and potential errors) will be shown and retained as long as a new run command is not sent.

You may send many concurent instructions with successive curl. For example
```bash
curl -X POST http://localhost:8080/run -H "Content-Type: application/json" -d '{"flags": "--cpu 10 --timeout 20s -vm 4"}'
curl -X POST http://localhost:8080/run -H "Content-Type: application/json" -d '{"flags": "--cpu 2 --timeout 60s -vm 4"}'
```
The maximum allowed number of run is set in the server application ```wrapper_api``` using the option ```--max-concurrent-jobs```.
Results are shown in batch. If you send 3 run that have overlaping runtime, these will be listed as [Job 1], [Job 2], [Job 3] with the information for each of them available in one request to the ```/results``` endpoint.
Once these are all finished and if a new command is sent, the Job counter is reset.

## Contributing
No contribution is expected. But feel free to submit issues or pull requests if you find any bugs or have suggestions for improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for details.