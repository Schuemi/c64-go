# c64-go
Frodo Port to ODROID-GO

Here is the precompiled Firmware file here: https://github.com/Schuemi/c64-go/releases

Please create these directories on your SD Card manually:
```
/roms/c64/bios
/odroid/data/c64
```
and put the BIOS files in /roms/c64/bios. You need the files "1541 ROM", "Basic ROM", "Char ROM" and "Kernal ROM". (without a pint between the name and "ROM"). For example you could get these files, if you download the Windows version of Frodo here: https://frodo.cebix.net/.
 

Your root folder to browse for games will be /roms/c64/. You can have subfolders in /roms/c64/.

In /odroid/data/c64 you can save keymapping files if you wish. The default keymapping is:
```
[KEYMAPPING]
UP = JST_UP
RIGHT = JST_RIGHT
DOWN = JST_DOWN
LEFT = JST_LEFT
SELECT = KEY_FM
START = KEY_R_S
A = JST_FIRE
B = KEY_SPC
```
The possible Key Mappings are:
JST_UP, JST_RIGHT, JST_DOWN, JST_LEFT, JST_FIRE, JST2_UP, JST2_RIGHT, JST2_DOWN, JST2_LEFT JST2_FIRE, KEY_SPC, KEY_CUD, KEY_F5, KEY_F3, KEY_F1, KEY_CLR, KEY_DEL, KEY_SHL, KEY_SHR, KEY_HOM, KEY_R_S, KEY_COMM, KEY_CTL, KEY_BAK, KEY_POUND, KEY_SLO, KEY_RESTORE, KEY_FM or any digit or letter.

The KEY_FM is a spezial button (default on SELECT). This is the fast mode button. If you press this button, the c64 emulation will run very fast, so you can skip long loading times.

Some other key declarations:
KEY_SPC Space key
KEY_CUD cursor UP/DOWN
KEY_SHL Shift on the left
KEY_SHR Shift on the right
KEY_R_S the run/stop key
KEY_HOM the "CLR HOME" key
KEY_CTL the CTRL key
KEY_BAK the button with the arrow to the left



You can use for every game a custom mapping file. Put the file in /odroid/data/c64. It schould have the name [GAME].ini. For example if your gamefiles name is GTA3.d64 the keyfilename has to be GTA3.ini.

Also you can have a default setting. Go into /odroid/data/c64, if you have ever started c64 there should be a file called "config.ini". In this file you can add your defualt keymapping the same way you would add a key mapping for a game.




Gamesaves are saved in the same directory as the game itself. It is calles [GAME].sta and is compatible to every other Frodo port. So you can continue playing on WII, Android etc...

# Virtual Keyboard:

You can open a virtual keyboard by pressing an hold the "A" button and then the "Menu" button. On the virtual keyboard you can use the "A" or the "B" button to press a key. If you are holding a button, this button will also be holded in the emulator. So if you go to the "shift" key, press and hold "A" and then while pressing "A" go to the "1" key and press "B"  you will write a "!". Because on the real C64 if you would press shift + 1 you will also write a "!"



# 1541 Emulation

In the Menu you can activate the 1541 emulation (the 5 1/4 inch disc drive of the C64). If you activate this, the time to load a game will be much longer. But some games will only work with this option. The 1541 emulation will fall back to "No" if you reset. When you load a savegame, Frodo automatically sets or removes option 1541, depending on what was set at the time of saving.

# Compatibility

Frodo has two modes: PC mode and SC mode. PC mode is much faster, but cannot play as many games as SC mode. The ODROID GO is too slow to run the SC mode. So not all games will work, but many! If a game doesn't work, please look for another version of this game. There is hope that another version from another release group will work.


# Next:

The next things I'm planning are:

- Multiplayer mode as in fMSX


# Donations 

Donations are very welcome. I have long wondered if I should add a donate button here or not. But, what the hell, if one or the other beer should come out for my troubles here, I like to drink one to you! Thanks a lot!

One beer donation (3.50€):
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HTTLDQT45WAPC" rel="some text">![3.50](https://www.paypalobjects.com/en_GB/i/btn/btn_donate_LG.gif)]</a>

Two beer donations (5.00€):
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=V32J6VX9Y7QQJ" rel="some text">![5.00](https://www.paypalobjects.com/en_GB/i/btn/btn_donate_LG.gif)]</a>

Donate a six-pack of beer. :) (10.00€):
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=QM4DJECCZTKPY" rel="some text">![10.00](https://www.paypalobjects.com/en_GB/i/btn/btn_donate_LG.gif)]</a>





Hint:
If you find any spelling or grammatical mistakes, please tell me. My english could be better. Thank you.

