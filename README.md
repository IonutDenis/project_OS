Treasure Manager
This program allows users to manage treasure hunts by storing treasures in a structured way. Users can add, list, view, and remove treasures in specific hunts. Each action performed by the user is logged in a text file, logged_hunt, within the respective hunt directory.

Features
Add Treasure: Add a treasure to a hunt with user details, location, clue, and value.

List Treasures: List all treasures within a specific hunt.

View Treasure: View a specific treasure's details by its ID.

Remove Treasure: Remove a specific treasure by its ID from a hunt.

Logging: All operations are logged in a logged_hunt text file in the hunt directory.

Requirements
A Unix-like operating system (Linux, macOS).

GCC compiler (for compiling the C code).

Basic familiarity with using the terminal/command line.

Compilation
To compile the program, use the following command in the terminal:
gcc -o treasure_manager treasure_manager.c

Usage
The program supports the following commands:

Add a Treasure:
./treasure_manager add <hunt_id>

List All Treasures in a Hunt:
./treasure_manager list <hunt_id>

View a Specific Treasure:
./treasure_manager view <hunt_id> <treasure_id>

Remove a Specific Treasure:
./treasure_manager remove <hunt_id> <treasure_id>
