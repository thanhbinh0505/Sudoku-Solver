# 🧩 Sudoku Solver using Dancing Links (Algorithm X – Donald Knuth)

An efficient and elegant Sudoku solver that leverages **Dancing Links** and **Algorithm X**, a powerful method introduced by Donald Knuth to solve **Exact Cover** problems. This project models Sudoku constraints as an exact cover matrix and solves them using a highly optimized recursive backtracking approach.

## 🚀 Features

- ✅ Solves standard 9×9 Sudoku puzzles accurately and quickly  
- 🧠 Implements Algorithm X with Dancing Links for optimal performance  
- 🧩 Models Sudoku as an exact cover problem  
- 🔍 Detects multiple solutions and checks for puzzle validity  
- 🛠️ Easy to integrate, extend, or use as a learning tool  

## 🕹️ Usage Instructions

You can interact with the Sudoku board using your keyboard:

- **Arrow Keys (↑ ↓ ← →)** – Move the selection across cells in the corresponding direction  
- **Home** – Jump to the **first cell** of the current row  
- **End** – Jump to the **last cell** of the current row  
- **Page Up** – Jump to the **topmost cell** of the current column  
- **Page Down** – Jump to the **bottommost cell** of the current column  
- **Space / Delete / Backspace** – Clear the currently selected cell  

Use number keys (1–9) to input values into cells. Once the board is filled or partially filled, you can run the solver to complete the puzzle or validate your solution.

<img width="547" height="610" alt="Sudoku" src="https://github.com/user-attachments/assets/9049a914-e0e9-41c6-bcdb-f0fe25dd4848" />
<img width="547" height="610" alt="Puzzle" src="https://github.com/user-attachments/assets/cd7918e2-95e1-4644-94ae-291cb16659cf" />
<img width="547" height="610" alt="Solution" src="https://github.com/user-attachments/assets/75e2f6ea-6060-4778-9d04-968121966d0e" />

## 🛠️ Build Instructions

You can compile the project using either **Microsoft Visual C++ (MSVC)** or **MinGW (g++)**, depending on your development environment.

### ✅ Compile with Microsoft Visual C++ (MSVC)

```bash
rc /r sudoku.rc
cl /EHsc sudoku.cpp sudoku.res user32.lib gdi32.lib
```
> Ensure `rc.exe` and `cl.exe` are available in your environment  
> (e.g., use the **Developer Command Prompt for Visual Studio**).

### ✅ Compile with MinGW (g++)

```bash
windres sudoku.rc -O coff -o sudoku.o
g++ sudoku.cpp sudoku.o -luser32 -lgdi32 -mwindows -o sudoku.exe -static -static-libgcc -static-libstdc++ -pthread
```
> 💡 Make sure `windres` and `g++` are in your system `PATH` if you're using **MinGW**.

---

## 📦 Use Cases

- 🧪 Educational tool for studying backtracking and constraint solving  
- 🔢 Fast Sudoku solving engine for apps or games  
- 💻 Demonstration of Algorithm X and Dancing Links in action  

---

## 📚 References

- Donald E. Knuth, *"[Dancing Links](https://arxiv.org/abs/cs/0011047)"*, _The Art of Computer Programming_
- Exact Cover problem and its applications in combinatorics and optimization  

---

## 📝 License

MIT License. Feel free to use, modify, and contribute!

