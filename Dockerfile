# Use an official Ubuntu as a parent image
FROM ubuntu:20.04

# Set environment variables to avoid user interaction during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    libboost-program-options-dev \
    stress-ng \
    && apt-get clean

# Set Boost environment variables
ENV BOOST_ROOT=/usr/local/boost
ENV BOOST_INCLUDEDIR=$BOOST_ROOT/include
ENV BOOST_LIBRARYDIR=$BOOST_ROOT/lib
ENV CPLUS_INCLUDE_PATH=${BOOST_INCLUDEDIR}
ENV LIBRARY_PATH=${BOOST_LIBRARYDIR}
ENV LD_LIBRARY_PATH=${BOOST_LIBRARYDIR}
    
# Create a directory for the application
WORKDIR /app

# Copy the application source code to the container
COPY . /app

# Build the application
RUN mkdir build && cd build && cmake .. && make

# Expose the port the application runs on
EXPOSE 8080
USER root

# Run the application
ENTRYPOINT ["./wrapper_api"]