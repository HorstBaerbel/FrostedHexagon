import processing.serial.*;

Serial myPort;  // Create object from Serial class
String val;     // Data received from the serial port
String portName = "/dev/ttyACM0";
int portSpeed = 234000;
byte[] headerData = new byte[7];
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
  drawText(10, height - 10, "Frame time: " + frameTimeMs + "ms"); 
}

void setup()
{
  size(320, 320);
  font = createFont("Arial", 16, true); // Arial, 16 point, anti-aliasing on
  myPort = new Serial(this, portName, portSpeed);
  delay(3000); //<>//
}

void draw()
{
  if ( myPort.available() > 0) {
    // synchronize to 'L' in header
    while (myPort.available() > 0 && myPort.read() != 'L') {}
    //println("L received");
    // read rest of header
    while (myPort.available() < 7) {}
    myPort.readBytes(headerData);
    if (headerData[0] != 'E' || headerData[1] != 'D' || headerData[2] != 'S' ||
      headerData[3] != ',' || headerData[6] != ',') {
      return;
    }
    //println("Header received");
    // read size of data
    int numBytes = (((int)headerData[5]) << 8 | headerData[4]);
    int numLeds = numBytes / 3;
    // check if we need to re-allocate LED data
    if (ledData == null || ledData.length != numBytes) {
      ledData = new byte[numBytes];
    }
    //println("Size is " + numBytes + " = " + numLeds + " LEDs");
    // read LED data
    int bytesRead = 0;
    while (numBytes > bytesRead) {
       while (myPort.available() > 0 && numBytes > bytesRead) {
         ledData[bytesRead] = (byte)myPort.read();
         bytesRead++;
       }
    }
    frameCount++;
    if (frameCount > 20) {
      frameTimeMs = (millis() - lastFrameTimeMs) / frameCount;
      lastFrameTimeMs = millis();
      frameCount = 0;
    }
    //println(millis() + "ms - " + bytesRead + " Bytes received.");
    drawLEDs(ledData, numLeds);
  }
}