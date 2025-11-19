# Project 03: State Minimization
**Student ID:** B11330203

## Compilation Instructions (Linux/WSL)
This project uses CMake for the build process. Please follow the steps below to compile the program:

1. **Create a build directory and enter it:**
   ```bash
   mkdir build
   cd build
   ```

2. **Generate the Makefiles using CMake:**
   ```bash
   cmake ..
   ```

3. **Compile the executable:**
   ```bash
   make
   ```

After successful compilation, the executable file `smin` will be generated in the **`bin`** directory at the project root (i.e., `../bin/smin`).

---

## Usage Example
To run the state minimization program, execute `smin` from the `bin` directory.

**Syntax:**
```bash
<path_to_executable>/smin <input_kiss> <output_kiss> <output_dot>
```

**Example (running from the project root):**
```bash
./bin/smin B11330203.kiss output.kiss output.dot
```

## Visualization
To convert the output DOT file to a PNG image (requires Graphviz):
```bash
dot -Tpng output.dot > output.png
```