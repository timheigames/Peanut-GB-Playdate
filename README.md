# Peanut-GB-Playdate

![peanutGB_icon](https://user-images.githubusercontent.com/102014001/167331546-b2aa5516-1a8f-4131-8196-3599cc5fb73e.png)

A port of the Peanut-GB Gameboy emulator for Playdate. [Peanut-GB](https://github.com/deltabeard/Peanut-GB) is a header-only C Gameboy emulator created by [deltabeard](https://github.com/deltabeard). I have tried to optimize as much as I could for Playdate, but most games still only run at 20-30 fps. Because the Gameboy ran at 60 fps, this means games will feel like you are playing at half speed or slower. This port has working audio, a rom picker, and save support (not save states).

## Installing
- Download the pdx ZIP from the [Releases](https://github.com/timheigames/Peanut-GB-Playdate/releases) section.
- Install it to your Playdate either through online sideloading or through USB and the Playdate Simulator.
- You will have to place roms in either `Data/com.timhei.peanutgb/` (USB sideload), or `Data/user.xxx.peanutgb/` (website sideload).
- To access the Data folder, plug your Playdate into a computer and hold `LEFT+MENU+LOCK` at the Playdate homescreen.
- Roms must end in ".gb" or ".gbc", or they will not be recognized.
- Gameboy Color is technically not supported, although sometimes games might be backwards compatible with the original Gameboy.

## Playing
- Controls are as expected, except start/select is mapped to forward crank/backward crank. You only need to move the crank a small amount to trigger it. There is a small cooldown to prevent multiple triggers in a short time period.
- In the Playdate menu you can select from Save, Reset, and Choose ROM. Save is not a save state, but rather saving the in-game save to the disk. Saving also automatically happens when quitting the app or changing ROMs. Saves are automatically loaded on ROM launch. If you want to delete a save, you will have to delete the `.sav` file from the ROM folder.

## License
This project is licensed under the MIT License.
