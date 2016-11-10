FrostedHexagon
========
A hexagonal LED display made from frosted glass and RGB LEDs. It uses an Arduino Pro mini and a rotatry encoder for menu navigation. The power supply is a regular 5V USB cell phone charger.  
The animations used are based on the FastLED/DemoReel100 sketch, but modified and converted so their speed is adjustable through the menu.

![FrostedHexagon in semi-finished state](frosthex.jpg?raw=true)  

The [Fritzing](http://fritzing.org/) circuit layout can be found in [FrostedHexagon.fzz](FrostedHexagon.fzz) and the necessary Arduino source code in [FrostedHexagon/FrostedHexagon.ino](FrostedHexagon/FrostedHexagon.ino).  

![Fritzing cicuit](fritzing_circuit.png?raw=true)  

Simulation
========
There is a sort-of simulation processing sketch in [Simulation/Simulation.pde](Simulation/Simulation.pde). 
You can use this by connecting an Arduino or whatever your platform is to a serial port. 
No other hardware is needed.
Turn on the "#define ENABLE_LED_EMULATION" in the Arduino code and set the correct serial port in the processing sketch. 
When you start your Arduino and run the processing sketch, the LED data your Arduino calculated, will be transfered to and displayed in the Processing window. 
You can use the processing GUI to adjust all the values on the Arduino and instantly see the results. 
The timing might not bee 100% correct, but it's an easy way to try out new pattern algorithms when you don't have the real device handy.  

License
========
[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](LICENSE.md).  
FastLED uses the [MIT license](https://opensource.org/licenses/MIT), see [LICENSE](https://github.com/FastLED/FastLED/blob/master/LICENSE).  

Usage
========
The LED display starts with the settings used before powering it off. Press the encoder once to show the menu. 
Now you can turn the encoder left / right to select the menu items. The pixel in the top row of the display shows your selection:  
 * Left / Red - Change animation pattern + pattern cycle mode
 * Center / Green - Change display brightness
 * Right / Blue - Change animation speed

After selecting the menu item, press the encoder again and the menu item will be activated and the pixel will start to blink. 
You can now adjust the value of the parameter you selected by turning the rotary encoder left / right.  
When choosing animation patterns, selecting the first / leftmost entry will automatically turn on animation cycle mode. This will cycle through all animation patterns, switching in 60s intervals. Specific animation patterns start with the second entry. This will automatically turn cycle mode off and show only the selected pattern. Currently six animation patterns are available.  
The settings will automatically be saved when the menu is closed, which happens after not interacting with the menu for 10s.

I found a bug or have a suggestion
========
The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** You can also contact me via email if you want to.
