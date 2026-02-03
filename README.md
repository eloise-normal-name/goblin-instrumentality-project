# Goblin Stream

Windows C++ application using MSVC and MSBuild targeting the latest C++ standard.

## Requirements

- Visual Studio 2022 Community (or later)
- Windows 10/11 x64
- VS Code with C/C++ extension (optional, for IDE-based development)

## Project Structure

```
goblin-stream/
├── .github/copilot-instructions.md    # Project standards and requirements
├── .clang-format                      # Google C++ code style configuration
├── .vscode/                           # VS Code configuration
├── bin/Debug/                         # Debug build output
├── bin/Release/                       # Release build output
├── obj/                               # Intermediate build artifacts (git-ignored)
├── include/                           # Public header files
├── src/main.cpp                       # Application entry point (WinMain)
├── goblin-stream.vcxproj              # MSBuild project file
└── goblin-stream.sln                  # Visual Studio solution
```

## Build

### Using VS Code

- **Build Debug**: Press `Ctrl+Shift+B`
- **Build & Run Debug**: Press `Ctrl+Shift+T`
- **Debug with Debugger**: Press `F5`

### Using Visual Studio

1. Open `goblin-stream.sln` in Visual Studio
2. Select `Debug` or `Release` configuration and `x64` platform
3. Press `Ctrl+Shift+B` to build

### Using Command Line

```powershell
# Build Debug
msbuild goblin-stream.vcxproj /p:Configuration=Debug /p:Platform=x64

# Build Release
msbuild goblin-stream.vcxproj /p:Configuration=Release /p:Platform=x64

# Run Debug build
.\bin\Debug\goblin-stream.exe

# Run Release build
.\bin\Release\goblin-stream.exe
```

## Code Standards

All code must follow the standards defined in `.github/copilot-instructions.md`:

- **C++23** standard with self-documenting code
- **Google C++ style** enforced by `.clang-format`
- **Windows API** preferred over STL when straightforward
- **Static linking** (MultiThreaded runtime `/MT`)
- **No external libraries** (C++ standard library and Windows APIs only)
- **No multi-threading**

## Development

1. Create a feature branch: `git checkout -b feature/your-feature`
2. Make changes and keep code self-documenting
3. Format code: Automatic on save in VS Code (or manually with clang-format)
4. Build and test: `Ctrl+Shift+T` in VS Code
5. Commit and push your branch

## Resources

- [Windows API Documentation](https://docs.microsoft.com/en-us/windows/win32/api/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [MSVC Documentation](https://docs.microsoft.com/en-us/cpp/)
