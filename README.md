# Ring Buffer Library

A high-performance, thread-safe ring buffer implementation in C with zero-copy read APIs and fragmented data handling.

## Features

- **Zero-copy read operations** using peek/commit pattern
- **Fragmented data handling** for optimal memory utilization
- **Multiple buffer instances** with handle-based management
- **Configurable buffer sizes** up to 10MB per buffer
- **Memory safety** with proper error handling and validation
- **Static library** for easy integration

## Architecture

The library provides a ring buffer implementation with the following key components:

- **Buffer Management**: Create/destroy buffer instances with unique handles
- **Write Operations**: Sequential write with automatic wrap-around
- **Read Operations**: Zero-copy peek/commit pattern for efficient data access
- **Fragmentation Support**: Handles data that wraps around buffer boundaries
- **Memory Management**: Safe allocation/deallocation with leak prevention

## API Overview

### Initialization
```c
void Rb_InitModule(void);           // Initialize the buffer module
void Rb_DeinitModule(void);         // Cleanup all resources
```

### Buffer Management
```c
cBool Rb_CreateBuffer(cU64_t bufferSizeInBytes, cI32_t *bufferHandle);
cBool Rb_DestroyBuffer(cI32_t *bufferHandle);
```

### Data Operations
```c
cBool Rb_WriteToBuffer(cI32_t bufferHandle, const cU8_t *data, cU64_t dataBytes);
cBool Rb_PeekRead(cI32_t bufferHandle, cU8_t **readPtr, cU64_t *dataBytes);
cBool Rb_CommitRead(cI32_t bufferHandle, cU64_t dataBytes);
```

### Buffer Status
```c
cU64_t Rb_GetUnreadIndexCount(cI32_t bufferHandle);
cBool Rb_GetFreeSpace(cI32_t bufferHandle, cU64_t *freeSpace);
```

## Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Generate Makefiles
cmake ..

# Build the static library
make

# Install library and headers (default: ./install)
make install

# Clean build artifacts
make extra_clean
```

### Build Output
- **Static Library**: `install/lib/liblibbuffer.a`
- **Headers**: `install/include/*.h`

### Custom Install Location
```bash
cmake -DCMAKE_INSTALL_PREFIX=/your/custom/path ..
```

## Usage Example

```c
#include "ringBuffer.h"

int main() {
    // Initialize the module
    Rb_InitModule();

    // Create a buffer
    cI32_t handle;
    if (Rb_CreateBuffer(1024, &handle) == c_TRUE) {
        // Write data
        const char *data = "Hello, World!";
        Rb_WriteToBuffer(handle, (cU8_t*)data, strlen(data));

        // Read data (zero-copy)
        cU8_t *readPtr;
        cU64_t dataSize;
        if (Rb_PeekRead(handle, &readPtr, &dataSize) == c_TRUE) {
            // Process data without copying
            printf("Read: %.*s\n", (int)dataSize, readPtr);

            // Commit the read
            Rb_CommitRead(handle, dataSize);
        }

        // Cleanup
        Rb_DestroyBuffer(&handle);
    }

    // Deinitialize
    Rb_DeinitModule();
    return 0;
}
```

## Configuration

Current compile-time configurations in `ringBuffer.c`:

- **MAX_BUFFER_HANDLE**: 10 (maximum concurrent buffer instances)
- **MAX_ALLOWED_BUFFER_SIZE_IN_BYTES**: 10MB per buffer
- **MAX_DATA_INDEX**: 1000 (maximum data chunks per buffer)

## Project Structure

```
buffer-lib/
├── src/
│   ├── ringBuffer.h         # Main API header
│   ├── ringBuffer.c         # Ring buffer implementation
│   └── common/
│       ├── common_stddef.h  # Type definitions
│       ├── common_def.h     # Common macros and utilities
│       ├── common_def.c     # Utility implementations
│       ├── common_utils.h   # Time utilities header
│       └── common_utils.c   # Time utilities implementation
├── CMakeLists.txt           # Build configuration
└── README.md               # This file
```

## TODO

### High Priority
- **Thread Safety**: Add mutex-based synchronization for multi-threaded access
- **Partial Read Support**: Allow reading partial data from a chunk
- **Copy-based Read APIs**: Add traditional copy-to-buffer read functions
- **Example Programs**: Create comprehensive usage examples

### Medium Priority
- **User-provided Memory**: Support for pre-allocated buffer memory
- **Multiple Readers/Writers**: Support concurrent access patterns
- **Runtime Configuration**: Make limits configurable at runtime
- **Performance Benchmarks**: Add performance testing and optimization
- **Documentation**: Generate API documentation with Doxygen

### Build System Enhancements
- **pkg-config**: Add pkg-config support for easier integration
- **CMake Export**: Add proper CMake export configuration
- **Cross-compilation**: Support for different target architectures
- **Sanitizers**: Add AddressSanitizer/ThreadSanitizer support in debug builds

