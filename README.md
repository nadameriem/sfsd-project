Description
The Simplified File Management System (FMS) is a simulator that replicates basic file management functionalities such as memory allocation, file creation, and record operations. It serves as an academic project designed to demonstrate core file management principles.

Features
- User authentication (login and signup)
- File creation and loading into memory
- Record insertion, search, and deletion (logical and physical)
- File defragmentation and renaming
- Secondary memory compaction and status display
- Metadata management for stored files

Technologies Used
- Programming Language: C
- Development Environment: Any C-compatible IDE (e.g., Visual Studio Code, Code::Blocks)

Installation
1. Clone the repository:
   git clone https://github.com/nadameriem/FMS.git

2. Navigate to the project directory:
   cd FMS

3. Compile the code:
   gcc -o FMS main.c file_operations.c memory_management.c

4. Run the simulator:
   ./FMS

Usage
1. Launch the program using the command provided above.
2. Authenticate by either logging in or signing up.
3. Use the interactive menu to perform the following operations:
   - Initialize secondary memory blocks.
   - Create, load, delete, or rename files.
   - Insert, search, or delete records.
   - Defragment and compact secondary memory.
   - View file metadata and memory status.
4. Follow on-screen prompts for input and instructions.

Project Structure
FMS/
- src/: Contains main and functional code files.
- include/: Header files for modular programming.
- tests/: Test cases for functionality validation.
- README.md: Project documentation.
- Makefile: Script for compiling the project.

Contributions
Contributions are encouraged! Feel free to report issues, request features, or submit pull requests to improve the project.

Limitations
- Designed for educational purposes and may not handle large-scale file operations efficiently.
- Metadata updates are not fully dynamic.

Strengths
- Easy-to-understand implementation of basic file management principles.
- Flexible and modular code for further development.

License
This project is licensed under the MIT License. See the LICENSE file for more details.

Contact
For questions or feedback, reach out to:
- Email: wissamlouail0105@gmail.com
- Email: moussaouimeriemnada@gmail.com
