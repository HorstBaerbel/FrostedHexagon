import processing.serial.*;
import controlP5.*;

ControlP5 cp5;
Knob knobPattern;
Toggle toggleCycle;
Slider sliderBrightness;
Slider sliderSpeed;
boolean brightnessChanged = false;
boolean speedChanged = false;
boolean patternChanged = false;
boolean cyclingChanged = false;
Serial myPort;  // Create object from Serial class
String val;     // Data received from the serial port
String portName = "/dev/ttyACM0";
int portSpeed = 115200; //230400;
enum SerialState { WaitForHeader, WaitForSize, WaitForData };
SerialState serialState = SerialState.WaitForHeader;
byte[] messageHeader = {'L', 'E', 'D', 'S', ','};
byte[] headerData = new byte[4];
byte[] sizeData = new byte[3];
byte[] ledData;
PFont font;
int frameCount = 0;
int lastFrameTimeMs = millis();
int frameTimeMs = 0;

float hexSideLength = 36;
float hexHeight = round(sin(30.0 * PI / 180.0) * hexSideLength);
float hexRadius = round(cos(30.0 * PI / 180.0) * hexSideLength);
float hexRectangleHeight = hexSideLength + 2.0 * hexHeight;
float hexRectangleWidth = 2.0 * hexRadius;
float[][] hexPositions = {
  {1, 0}, {2, 0}, {3, 0},
  {3.5, 1}, {2.5, 1}, {1.5, 1}, {0.5, 1}, 
  {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, 
  {3.5, 3}, {2.5, 3}, {1.5, 3}, {0.5, 3}, 
  {1, 4}, {2, 4}, {3, 4}}; // LEDs in data order (how they come from the Arduino)

float[][] hexVertex = { 
  {0, hexRectangleHeight/2}, {-hexRadius, hexSideLength/2},
  {-hexRadius, -hexSideLength/2}, {0, -hexRectangleHeight/2},
  {hexRadius, -hexSideLength/2}, {hexRadius, hexSideLength/2}};

void drawHexagon(float x, float y) {
  pushMatrix();
  translate(x, y);
  beginShape();
  for (int i = 0; i < 6; i++) {
    pushMatrix();
    vertex(hexVertex[i][0], hexVertex[i][1]);
    popMatrix();
  }
  endShape(CLOSE);
  popMatrix();
}

void drawText(float x, float y, String text)
{
  textFont(font, 16);
  fill(255, 255, 255);
  text(text, x, y);
}

void drawLEDs(byte[] ledData, int numLeds)
{
  background(20, 20, 20);
  noStroke();
  for (int i = 0; i < numLeds; ++i) {
    // get color from data
    fill(ledData[i*3], ledData[i*3+1], ledData[i*3+2]);
    //fill(random(255), random(255), random(255));
    // get position from array and draw
    float px = hexPositions[i][0] * hexRectangleWidth + hexRectangleWidth / 2;
    float py = hexPositions[i][1] * (hexSideLength + hexHeight) + hexSideLength;
    drawHexagon(px, py);
  }
}

//------------------------------------------------------------------------- //<>//

void setup()
{
  size(480, 300);
  cp5 = new ControlP5(this);
  knobPattern = cp5.addKnob("pattern")
    .setRange(0,3)
    .setValue(0)
    .setNumberOfTickMarks(3)
    .snapToTickMarks(true)
    .setPosition(330, 10)
    .setRadius(30)
    .setDragDirection(Knob.HORIZONTAL)
    .addListener(new ControlListener() {
      public void controlEvent(ControlEvent theEvent) {
        patternChanged = true;
      }
    });
  toggleCycle = cp5.addToggle("cycle")
    .setValue(0)
    .setPosition(410, 20)
    .setSize(40, 40)
    .addListener(new ControlListener() {
      public void controlEvent(ControlEvent theEvent) {
        cyclingChanged = true;
      }
    });
  sliderBrightness = cp5.addSlider("brightness")
    .setPosition(330, 100)
    .setSize(80, 20)
    .setRange(32, 255)
    .setValue(96)
    .setColorCaptionLabel(color(255,255,255))
    .addListener(new ControlListener() {
      public void controlEvent(ControlEvent theEvent) {
        brightnessChanged = true;
      }
    });
  sliderSpeed = cp5.addSlider("speed")
    .setPosition(330, 140)
    .setSize(80, 20)
    .setRange(1, 255)
    .setValue(128)
    .setColorCaptionLabel(color(255,255,255))
    .addListener(new ControlListener() {
      public void controlEvent(ControlEvent theEvent) {
        speedChanged = true;
      }
    });
  //font = createFont("Arial", 16, true); // Arial, 16 point, anti-aliasing on
  myPort = new Serial(this, portName, portSpeed);
  myPort.clear();
  myPort.write('p'); myPort.write(0);
  myPort.write('c'); myPort.write(0);
  myPort.write('b'); myPort.write(96);
  myPort.write('s'); myPort.write(128);
}

void draw()
{
  if (serialState == SerialState.WaitForHeader) {
    // find 'L' first
    if (myPort.available() > headerData.length && myPort.read() == messageHeader[0]) {
      // read rest of header
      myPort.readBytes(headerData);
      if (headerData[0] == messageHeader[1] && 
        headerData[1] == messageHeader[2] && 
        headerData[2] == messageHeader[3] && 
        headerData[3] == messageHeader[4]) {
          //println("Header received");
          serialState = SerialState.WaitForSize;
      }
    }
  }
  if (serialState == SerialState.WaitForSize) {
    if (myPort.available() >= sizeData.length) {
      // read size information
      myPort.readBytes(sizeData);
      if (sizeData[2] == ',') {
        int numBytes = (((int)sizeData[1]) << 8 | sizeData[0]);
        // check if we need to re-allocate LED data
        if (ledData == null || ledData.length != numBytes) {
          ledData = new byte[numBytes];
        }
        //println("Size is " + numBytes + " Bytes");
        serialState = SerialState.WaitForData;
      }
    }
  }
  if (serialState == SerialState.WaitForData) {
    if (myPort.available() >= ledData.length) {
      // read LED data
      myPort.readBytes(ledData);
      //println(millis() + "ms - " + bytesRead + " Bytes received");
      frameCount++;
      if (frameCount > 20) {
        frameTimeMs = (millis() - lastFrameTimeMs) / frameCount;
        lastFrameTimeMs = millis();
        frameCount = 0;
        surface.setTitle("Simulation - Frame time: " + frameTimeMs + "ms");
      }
      int numLeds = ledData.length / 3;
      drawLEDs(ledData, numLeds);
      serialState = SerialState.WaitForHeader;
      // check if we have already too many bytes pending
      int maxMessageLength = ledData.length + messageHeader.length + sizeData.length;
      if (myPort.available() > maxMessageLength) {
        myPort.readBytes(maxMessageLength);
      }
    }
  }
  // check if we have to send new parameters
  if (brightnessChanged) {
    brightnessChanged = false;
    byte value = (byte)round(sliderBrightness.getValue());
    myPort.write('b'); myPort.write(value);
  }
  if (speedChanged) {
    speedChanged = false;
    byte value = (byte)round(sliderSpeed.getValue());
    myPort.write('s'); myPort.write(value);
  }
  if (patternChanged) {
    patternChanged = false;
    byte value = (byte)round(knobPattern.getValue());
    myPort.write('p'); myPort.write(value);
  }
  if (cyclingChanged) {
    cyclingChanged = false;
    byte value = (byte)round(toggleCycle.getValue());
    myPort.write('c'); myPort.write(value);
  }
}