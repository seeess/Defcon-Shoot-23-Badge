# Defcon 23 Shoot Badge
![badge image](http://i.imgur.com/w0be2bUl.jpg "green display = volunteer, yellow = normal, red = black badge")
****
I didn’t get any major sponsors, but Gigs who runs ballistictools.com did step up to help keep the cost down.

I’m covering costs for all development, programmer, original prototype, along with fronting thousands for the badges that thankfully ended up working. My goal is to break even on this project, and provide something fun for everyone to play with. Especially because Defcon’s non-electronic badge years are lame.

## Basics:
* Put the batteries in, don’t screw this up. I’m too lazy and cheap to add a diode.
* On power up, the badge does a little startup sequence showing “Defcon” “Shoot”, then counts up to 9 for each digit.
* During the startup sequence you can hold the start button to briefly see the code version running on the badge
* After startup the badge will be in the default shot counter mode (unless you already switched modes). By default the microphone threshold to count a shot is low so clapping should increment the counter. If it isn’t incrementing, increase the threshold to 40. (More on how to change the threshold later)
* There are two buttons, a microphone, and a tilt sensor. 
  * The “select” button brings up the options menu in most cases. 
  * The “start” button confirms a selection in most cases.
* To save on batteries you can put the badge to sleep, press and hold both buttons for ~2 seconds. Press and hold just the start button to wake the badge back up (a watchdog checks in every 2 seconds or so then goes back to sleep)
* In some modes there is a crude VU/dB meter, when a loud sound happens the decimal points will light up from left to right, and fall off if the sound does not remain loud. 
* To clear all saved settings in flash (or if something isn't working right), hold both buttons during startup. You should see 3 horizontal lines on each digit, followed by the normal startup.

## Option Menu:
****
*Note: The decimal points in the options menus crawl left to right, and give you a rough idea of where you are in the options menu.*
### 1. “count” = Count Up/Down Mode: 	
Continuously increments or decrements the display at based on the speed setting.
Pressing start while in this mode will switch between count up and count down (it also resets the counter).
You can set the value of the counter by using “setdig” mode.
### 2. “timer” = Shot Timer Mode:
This mode begins with a “Press Start” indication, followed by a random blackout period. Then the timer begins counting up. Pressing start during the blackout period will immediately start the timer.

The display will read “0. ##.##”
The leftmost digit is the shot number, it starts at “0” and counts each shot up to 15 (“F”). The four digits on the right display the time in 10’s of seconds, seconds, tenths of seconds, and hundredths of seconds. The maximum time for each shot is 99 seconds.

After a shot is detected the elapsed time between that current shot and the last shot is stored. The timer then resets to count the time between the next shot.

Remember to set the microphone threshold to the desired value, and don’t enable the shot counter’s quickmute setting or shots will not be counted.

You can review the time stored for each shot by pressing the start button (this will also stop the timer). 
Pressing start when no shots have been counted just resets the timer back to 00.00

The speed, tilt, and setdig settings have no effect in this mode. 
### 3. “ Shot “ = Shot Counter Mode (default mode):
When a noise breaks the microphone threshold the shot counter is incremented and saved to flash.
Pressing the start button toggles a quickmute/unmute (this is overwritten and superseded by any threshold changes)
To adjust the shot counter value use the setdig option.

The speed setting has no effect in this mode. 
### 4. “ rand “ = Random Display Mode:
Each digit’s segments are lit up randomly and changed at the rate of the speed setting.
Pressing start cycles through three sub-modes; scroll left to right, scroll right to left, and refresh all digits
The speed setting changes the scroll speed / refresh rate.

The tilt, threshold, and setdig settings have no effect in this mode
### 5. “ HYPE “ = Hype Mode:
Displays “Defcon” then “Shoot” in case you get too many questions on what the badge is for.
The threshold, and setdig settings have no effect in this mode
### 6. “Audio” = “Audio Debug Mode:
Displays the max microphone 10-bit ADC reading since last refresh, updated at the rate of the speed setting.

*Explain it like I’m 5:* The maximum volume level the mic picks up, with a range of 0-1023. Slow the speed setting down to 0-2 for readability.
This mode can be used for determining what the threshold setting should be set to.
If you press and hold the start button during this mode the maximum level will not be reset until the start button is released.

The threshold, and setdig settings have no effect in this mode.
### 7. “Setdig” = Set Digits
Sets the value that is displayed, works in Count up, Count down, and Shot counter modes.

The current configurable digit is designated by being slightly brighter and a blinking decimal point.
The start button increments the current digit’s value by one (mod 10)
The select button moves to the next digit.
### 8. “ tilt ” = Tilt Sensor Toggle
Flip the tilt sensor logic. This works in most modes. Select it again if you fucked up.

The tilt sensor has a short debounce so when the badge is flapping around your neck the display doesn’t keep inverting the display back and forth. 
### 9. “thresh” = Microphone Threshold Adjustment
To give everyone the most control you can set the raw microphone ADC threshold.
The select button cycles through available values (step 20) with the highest value being “off” (no shots will be counted). You can press and hold this for faster adjustment. The decimal point is lit on the currently selected value.
The start button selects the displayed value.

How to configure: Switch to audio mode, lower the speed setting to 1-2 or press and hold start, perform the action you want the microphone to trigger on a few times (clap, fire a gun, whatever). Note the ADC reading. Go back to the threshold adjustment, and select a value a little under the readings you observed. 
### 10. “Speed “ = Speed Setting
The speed setting adjusts how fast count up/down/random/audio/temp/batt/sound/morse code modes change. This setting is exponential in most modes.
### 11. “bright" = Brightness setting
You can choose between “High” (default), “med” (medium), and “dim”. Lowering the display brightness will extend battery life. However I recommend you keep the badge in “High” mode when using the microphone, there seems to be something causing the mic ADC to read higher when digits aren’t lit at max brightness. 
### 12. “ Play “ = A submenu for video games and shit
####  1.	“react “ = Reaction Game
This game will automatically start (or press the start button to skip the intro). 
The screen will go off for a random amount of time. 
When the digits start incrementing press “start” as quickly as possible. The digits will stop incrementing, which will be your score.
If you press “start” before the counter starts you will fail.

The speed, threshold, and setdig settings have no effect in this mode. 
####  2. “dodge” = Dodge Game
You’re the little space ship line on the left, the buttons will move you up and down. 
Blocks will scroll in from the right that you must dodge.

You have one nuke for use at any time, indicated by the decimal point. To use the nuke, tap the microphone or clap/scream at the badge. The nuke will be used if the microphone picks up a sound louder than a threshold of 40.

The speed, threshold, and setdig settings have no effect in this mode. 
####  3. “ temp ” = temperature reading
Displays the current temperature (kind of). 
Even though this is using a 10-bit ADC, valid temperature steps are about 1 degree C apart
Each badge reads slightly differently, so there is an offset available to calibrate the temp. 

The temperature reading is based on the voltage supplied, I attempt to compensate for this by reading the battery voltage once when entering this mode. However each badge can have a different temperature slope based on temperature. Basically you’ll have to continuously calibrate it. 

To enter the offset mode press the start button, pressing the start button again will increment the offset value, and the select button will decrement the offset value. (you can press and hold for faster adjustment). 
This offset menu will automatically timeout and store the value currently displayed.

The threshold, and setdig settings have no effect in this mode. 
####  4. “ batt ” = battery reading
Displays the current voltage supplied to the badge.
This can be slightly off so there is an offset to adjust the reading.
To enter the offset mode press the start button, pressing the start button again will increment the offset value, and the select button will decrement the offset value. (you can press and hold for faster adjustment).
This offset menu will automatically timeout and store the value currently displayed. 
Adjusting the battery offset will affect the temperature calibration, since the battery voltage value is used for the temperature calculation.

The threshold, and setdig settings have no effect in this mode. 
####  5.  “Sound” = Sound level mode
Displays the microphone volume similar to an equalizer. 
When the tilt option is not set, a live volume level meter is displayed. The display is pulled up from the middle digits, with a display roll off determined by the speed setting.
To change the sensitivity of the microphone press the start button. The loudness threshold can be increased by pressing start or decreased by pressing select (setting 0-7). The decimal point is lit on the currently selected value. 

The loudness menu will automatically timeout and store the value currently displayed. 
Larger loudness values require a louder sound to move the display.

When the tilt option is set, all digits are displayed the same for external relay control. They are basically inverted from the normal mode, and when a sound is detected segments go off.

The threshold, and setdig settings have no effect in this mode. 
####  6. “ Clap “ = Clapper mode
Since I apparently don’t understand what feature creep means, I implemented a clapper mode. This is especially useful with an external relay to control lights or whatever else you want. 
The decimal point pin can be hooked to an external relay. Due to current sinking on the cathode size of the display, the decimal points are lit the opposite of what you might expect. [See the external relay control section for more information](https://forum.defcon.org/forum/defcon/dc23-official-unofficial-parties-social-gatherings-events-contests/dc23-official-and-unofficial-events/unofficial-defcon-shoot/15239-defcon-23-shoot-electronic-badge)

By default it takes two claps to change the state, to adjust this by press start to enter a claps configuration mode. The number of claps can be increased by pressing start or decreased by pressing select (1-F). This claps configuration mode will automatically time out and store the value currently displayed. 

The speed, and setdig settings have no effect in this mode. 
####  7. “MorSE” = Morse code mode
When entering this mode you are prompted to select the built in string to display (“Str ##”). Pressing select increments this value, pressing start selects the displayed string number. There are 23 built in strings listed below (numbers 0-22).

Selecting “ Cust ” is for custom string entry. You will see “PXX.CYY” displayed. The “XX” numbers  refer to the current character position in the string (50 characters max, 0-49), and “YY” refers to the current character you wish to set (digits 0-35 referring to a-z and 0-9, so 0=a, 25=z, 26=0, 35=9). The select button will scroll through available characters to insert into the string (YY above), the start button will add the current character indicated by “YY” to the string. When you are done entering characters select “ done “ which is shown after the 35th character.

The string pattern will now blink out in morse code. 
You can adjust the speed setting to increase or decrease the blink speed.
Hook this up to xmas lights or something after the shoot using the pin headers and a relay. Set the tilt option to invert the lighting which is what you need for external relay control.

The threshold, and setdig settings have no effect in this mode. 

#### Built in morse code strings:
    0:  defcon23
    1:  SOS
    2:  hack everything
    3:  break shit
    4:  nothing is impossible
    5:  fuck the NSA
    6:  I dont play well with others
    7:  what are you doing dave
    8:  youre either a one or a zero alive or dead
    9: danger zone
    10: bros before apparent threats to national security
    11: im spooning a Barrett 50 cal I could kill a building
    12: there is no spoon
    13: never send a human to do a machines job
    14: guns lots of guns
    15: its not that im lazy its just that i dont care
    16: PC load letter
    17: shall we play a game
    18: im getting too old for this
    19: censorship reveals fear
    20: the right of the people to keep and bear Arms shall not be infringed
    21: all men having power ought to be mistrusted
    22: when governments fear the people there is liberty
