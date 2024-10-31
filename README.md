# RTA2MID
Realtime Associates (David Warhol) (GB/GBC/GG) to MIDI converter

This tool converts music from Game Boy and Game Boy Color games using David Warhol (of Realtime Associates)'s sound engine to MIDI format. It also works with the Game Gear version of the sound engine since it uses the same sequence format, which appears to be a modified version of SEGA's US supplied driver.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain multiple banks of music (i.e. Return of the Jedi), you must run the program multiple times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.
To specify the system between Game Boy and Game Gear, you use the flag "GB" and "GG". If no such flag is present, it automatically selects Game Boy mode.

Examples:
* RTA2MID "Zoop (U) [!].gb" 4 GB
* RTA2MID "Star Wars - Super Return of the Jedi (U) [S][!].gb" 5 GB
* RTA2MID "Star Wars - Super Return of the Jedi (U) [S][!].gb" 6 GB
* RTA2MID "Zoop (U) [!].gg" 3 GG
* RTA2MID "Super Return of the Jedi (U) [!].gg" 5 GG
* RTA2MID "Super Return of the Jedi (U) [!].gg" 6 GG

This tool was based on my own reverse-engineering of the sound engine, with little disassembly involved. The sequence format happened to be practically identical between Game Boy and Game Gear, so it supports both systems.

The other included "prototype" tool, RTA2MID, as usual, prints out information about each song to TXT.

Supported games:

Game Boy:
* All-Star Baseball '99
* All-Star Baseball 2000
* All-Star Baseball 2001
* Barbie: Ocean Discovery
* BreakThru!
* Captain America and the Avengers
* Caterpillar Construction Zone
* Dick Tracy
* Frank Thomas' Big Hurt Baseball
* Heroes of Might and Magic
* Iron Man & X-O Manowar in Heavy Metal
* Mysterium
* Q-bert for Game Boy
* Skate or Die: Tour de Thrash
* Space Date
* Super Star Wars: Return of the Jedi
* Toxic Crusaders
* WildSnake
* Wordtris
* WordZap
* WWF Raw
* Zoop

Game Gear
 * The Berenstain Bears' Camping Adventure
 * Captain America and the Avengers
 * Frank Thomas' Big Hurt Baseball
 * Iron Man & X-O Manowar in Heavy Metal
 * NHL Hockey
 * Quest for the Shaven Yak Starring Ren Hoëk & Stimpy
 * Super Star Wars: Return of the Jedi
 * WildSnake
 * WWF Raw
 * Zoop

## To do:
  * Support for the NES version of the sound engine
  * GBS (and SGC?) file support
