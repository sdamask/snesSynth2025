import processing.serial.*;
import controlP5.*;

Serial teensyPort;
ControlP5 cp5;

String serialPortName = ""; // Will be selected automatically or manually

void setup() {
  println("Starting setup()...");
  size(600, 400); 
  surface.setTitle("SNES Synth Controller GUI");
  delay(500); // Wait half a second
  
  println("Listing Serial Ports...");
  String[] portList = Serial.list();
  println("Available Serial Ports:");
  printArray(portList);
  delay(500);
  
  println("Attempting port selection...");
  // --- Serial Port Setup ---
  // Attempt to find Teensy automatically (common names)
  for (String p : portList) {
    if (p.contains("usbmodem") || p.contains("COM") || p.contains("ttyACM")) { // Adjust as needed for your OS
       serialPortName = p;
       println("Auto-selected Teensy port: " + serialPortName);
       break;
    }
  }
  
  if (serialPortName.equals("")) {
     println("Teensy port not found automatically. Select manually or check connection.");
     // Optionally exit or provide manual selection here
     // For now, we'll try to open the first port if needed
     if (portList.length > 0) {
         serialPortName = portList[0];
         println("Attempting to use first available port: " + serialPortName);
     } else {
         println("ERROR: No serial ports found!");
         exit();
     }
  }
  println("Selected Port: " + serialPortName);
  delay(500);

  println("Attempting to open Serial Port: " + serialPortName);
  delay(1000); // Wait a second before trying to open
  
  // --- Temporarily remove try-catch for debugging ---
  // try {
  teensyPort = new Serial(this, serialPortName, 9600); 
  teensyPort.bufferUntil('\n'); 
  // } catch (RuntimeException e) {
  //   println("Error opening serial port " + serialPortName + ": " + e.getMessage());
  //   println("Make sure the Teensy is connected and the Serial Monitor in Arduino IDE is closed.");
  //   exit();
  // }
  println("Serial Port Opened Successfully.");
  delay(500);

  // --- GUI Setup ---
  println("Initializing ControlP5...");
  cp5 = new ControlP5(this);
  println("ControlP5 Initialized.");
  delay(500);
  
  println("Adding GUI Controls...");
  // Add GUI controls here
  cp5.addSlider("boogieRTick") 
     .setLabel("Boogie R Tick (Swing)")
     .setPosition(20, 30)      
     .setSize(200, 20)        
     .setRange(12, 20)         
     .setValue(16)            
     .setNumberOfTickMarks(9) 
     .setSliderMode(Slider.FLEXIBLE)
     ;

  println("GUI Setup Complete.");
  // No exit() here, setup finished successfully
}

void draw() {
  background(60); // Dark background
  // Update GUI elements or draw other things here if needed
}

// Function to send commands to Teensy
void sendCommand(String command) {
  if (teensyPort != null && teensyPort.available() >= 0) { // Check if port is valid
     println("Sending Command: " + command);
     teensyPort.write(command + '\n'); // Send command with newline
  } else {
     println("Error: Serial port not available.");
  }
}

// Serial Event Handler (receives messages from Teensy)
void serialEvent(Serial p) {
  String msg = p.readStringUntil('\n');
  if (msg != null) {
    msg = msg.trim();
    println("Received from Teensy: " + msg);
    // Add logic here later to parse messages from Teensy if needed (e.g., status updates)
  }
}

// GUI Event handler for the slider
void boogieRTick(int value) { // Function name matches slider name
  // Send the command when the slider value changes
  sendCommand("boogie_r_tick " + value);
}

/*
// Generic controlEvent handler (alternative)
void controlEvent(ControlEvent theEvent) {
  if (theEvent.isFrom("boogieRTick")) {
      int value = (int)theEvent.getController().getValue();
      sendCommand("boogie_r_tick " + value);
  }
}
*/ 