# Peanut-GB-Playdate
A port of the Peanut-GB Gameboy emulator for Playdate. [Peanut-GB](https://github.com/deltabeard/Peanut-GB) is a header-only C Gameboy emulator created by [deltabeard](https://github.com/deltabeard). I have tried to optimize as much as I could for Playdate, but most games still only run at 20-30 fps. Because the Gameboy ran at 60 fps, this means games will feel like you are playing at half speed or slower. This port has working audio, a rom picker, and save support (not save states).

## Installing
Simply download the pdx ZIP from the Releases section, and install it to your Playdate either through online sideloading or through USB and the Playdate Simulator. You will have to install roms to the Data/com.timhei.peanutGB folder. Roms must end in ".gb" or ".gbc", or they will not be recognize. Gameboy Color is technically not supported, although some of them are backwards compatible with the original Gameboy.

## License
This project is licensed under the MIT License.
