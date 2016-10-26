FrostedHexagon
========
A hexagonal LED display made from frosted glass and RGB LEDs. It uses an Arduino Pro mini and a rotatry encoder for menu navigation. The power supply is a regular 5V USB cell phone charger.  
The animations used are based on the FastLED/DemoReel100 sketch, but modified and converted so their speed is adjustable through the menu.

![FrostedHexagon in semi-finished state](frosthex.jpg?raw=true)  

The [Fritzing](http://fritzing.org/) circuit layout can be found in [FrostedHexagon.fzz](FrostedHexagon.fzz) and the necessary Arduino source code in [FrostedHexagon/FrostedHexagon.ino](FrostedHexagon/FrostedHexagon.ino).  

![Fritzing cicuit](fritzing_circuit.png?raw=true)  

License
========
[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](LICENSE.md).  
FastLED uses the [MIT license](https://opensource.org/licenses/MIT), see [LICENSE](https://github.com/FastLED/FastLED/blob/master/LICENSE).  

Usage
========
The LED display starts with the last configuration before powering it off. Press the encoder once to show the menu. 
Now you can turn the encoder left/right to select the menu items. The pixel in the top row of the display shows your selection:  
 * Left / Red - Change animation pattern + pattern cycle mode
 * Center / Green - Change display brightness
 * Right / Blue - Change animation speed

After selecting the menu item, press the encoder again and the menu item will be activated and the pixel will start to blink. 
You can now adjust the value of the parameter you selected by turning the rotary encoder left / right.  
When choosing animation patterns, selecting the first / leftmost choice / entry will automatically turn on animation cycle mode. This will cycle through all animation patterns, switching in 60s intervals. Currently six animation patterns are available.  Specific animation patterns start with the second choice / entry. This will automatically turn cycle mode off and show the selected pattern for an indefinite amount of time.  
The settings will be saved when the menu is closed, which happens automatically after not interacting with the menu for 10s.

I found a bug or have a suggestion
========
The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** You can also contact me via email if you want to.
