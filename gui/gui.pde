import processing.serial.*;
import controlP5.*;
import java.util.Map; // Import the Map interface

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
  
  int currentX = 20;
  int currentY = 30;
  int controlWidth = 200;
  int controlHeight = 20;
  int spacingY = 35;

  // --- Mode Selection ---
  cp5.addRadioButton("modeSelection")
     .setLabel("Active Mode")
     .setPosition(currentX, currentY)
     .setSize(20, 20)
     .setItemsPerRow(1)
     .setSpacingColumn(50)
     .setSpacingRow(5)
     .addItem("Standard", 0) // Value 0
     .addItem("Boogie", 1)   // Value 1
     .addItem("Rhythmic", 2) // Value 2
     .activate(0); // Default to Standard
  currentY += 80;

  // --- Boogie Timing Controls ---
  cp5.addLabel("Boogie Mode Settings")
     .setPosition(currentX, currentY).setSize(controlWidth, 15);
  currentY += 20;
  
  cp5.addSlider("boogieRTick") 
     .setLabel("Boogie R Tick (0-23)")
     .setPosition(currentX, currentY)      
     .setSize(controlWidth, controlHeight)        
     .setRange(0, 23) // Full tick range now
     .setValue(12)    // Default value (straight 8ths)
     .setNumberOfTickMarks(24)
     .setSliderMode(Slider.FLEXIBLE)
     .snapToTickMarks(true)
     .setId(1); // Assign an ID for mode checking
     ;
  currentY += spacingY;
  
  // --- Rhythmic Pattern Controls ---
  cp5.addLabel("Rhythmic Pattern Settings")
     .setPosition(currentX, currentY).setSize(controlWidth, 15);
   currentY += 20;
   
  cp5.addSlider("notesPerCycle")
     .setLabel("Notes per Cycle")
     .setPosition(currentX, currentY)
     .setSize(controlWidth, controlHeight)
     .setRange(1, 16) 
     .setValue(5)     
     .setNumberOfTickMarks(16)
     .setSliderMode(Slider.FLEXIBLE)
     .snapToTickMarks(true)
     .setId(2); // Assign an ID for mode checking
     ;
  currentY += spacingY;
  
  DropdownList durationList = cp5.addDropdownList("baseDuration")
                                  .setLabel("Base Duration")
                                  .setPosition(currentX, currentY)
                                  .setSize(controlWidth, 100) 
                                  ;
  // Populate the dropdown list (Value is the Tick Count)
  durationList.addItem("16th", 6.0f);
  durationList.addItem("8th Triplet", 8.0f);
  durationList.addItem("8th", 12.0f);
  durationList.addItem("Quarter Triplet", 16.0f);
  durationList.addItem("Quarter", 24.0f);
  durationList.addItem("Half", 48.0f);
  durationList.addItem("Whole", 96.0f);
  durationList.setValue(12.0f); // Default to 8th note for consistency?
  durationList.close(); 
  durationList.setId(3); // Assign an ID for mode checking
  currentY += 30; 
  
  println("GUI Setup Complete.");
  // Call initial update to set control visibility based on default mode (Standard)
  updateControlVisibility(0); 
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

// --- GUI Event Handlers --- 

// Mode Selection Radio Buttons
void modeSelection(int value) {
  String modeName = "standard";
  if (value == 1) modeName = "boogie";
  else if (value == 2) modeName = "rhythmic";
  println("Mode selection changed to: " + modeName + " (Value: " + value + ")");
  sendCommand("mode " + modeName);
  updateControlVisibility(value); // Show/hide relevant controls
}

// Boogie Swing Slider
void boogieRTick(int value) {
  println("Boogie R Tick slider changed to: " + value);
  sendCommand("boogie_r_tick " + value);
}

// Rhythmic Pattern: Notes Slider
void notesPerCycle(int value) {
   println("Notes per cycle slider changed to: " + value);
   sendPatternCommand(); // Send combined pattern command immediately
}

// Rhythmic Pattern: Duration Dropdown
void baseDuration(int index) { 
  println("--- baseDuration Event Handler --- Index: " + index);
  DropdownList durationDropdown = cp5.get(DropdownList.class, "baseDuration");
  
  // Check if index is valid before getting item
  if (index >= 0 && index < durationDropdown.getItems().size()) {
      // Get the item - it's stored as a Map
      Map selectedItem = durationDropdown.getItem(index);
      println("Selected Item Object (Map): " + selectedItem);
      println("Selected Item Class: " + selectedItem.getClass().getName());
      
      // Access data from the Map
      String itemName = (String) selectedItem.get("name"); // Key is "name"
      Object itemValue = selectedItem.get("value");       // Key is "value"
      float value = -1.0f; // Default if cast fails
      if (itemValue instanceof Number) {
          value = ((Number)itemValue).floatValue();
      }
      
      println("  Item Name: " + itemName);
      println("  Item Value (Ticks): " + value);
      
      // --- The following line would cause the original error ---
      // ControlP5.DropdownList.Item selectedItemWrong = durationDropdown.getItem(index); 
      
  } else {
      println("Invalid index received from dropdown: " + index);
  }
  println("-------------------------------------");
  println("Base duration dropdown changed.");
  sendPatternCommand(); // Send combined pattern command immediately
}

// Helper function to send the combined pattern command
void sendPatternCommand() {
  println("Sending updated pattern command...");
  int numNotes = (int)cp5.getController("notesPerCycle").getValue();
  float totalTicks = 48.0f; // Default
  try {
    DropdownList durationDropdown = cp5.get(DropdownList.class, "baseDuration");
    int selectedIndex = (int)durationDropdown.getValue(); 
    Map selectedItem = durationDropdown.getItem(selectedIndex);
    Object itemValue = selectedItem.get("value"); 
    if (itemValue instanceof Number) {
      totalTicks = ((Number)itemValue).floatValue();
    }
  } catch (Exception e) {
      println("ERROR retrieving dropdown value for pattern command: " + e.getMessage());
  }
  println("  Num Notes: " + numNotes + ", Total Ticks: " + totalTicks);
  String command = "pattern " + numNotes + " " + totalTicks;
  sendCommand(command);
}

// Helper to show/hide controls based on mode
void updateControlVisibility(int modeValue) {
   boolean isBoogie = (modeValue == 1);
   boolean isRhythmic = (modeValue == 2);
   
   // Use IDs to get controllers
   Controller boogieSlider = cp5.getController("boogieRTick");
   Controller notesSlider = cp5.getController("notesPerCycle");
   Controller durationDropdown = cp5.get(DropdownList.class, "baseDuration"); // Get specific type
   
   if(boogieSlider != null) boogieSlider.setVisible(isBoogie);
   if(notesSlider != null) notesSlider.setVisible(isRhythmic);
   if(durationDropdown != null) durationDropdown.setVisible(isRhythmic);
}

/*
// Generic controlEvent handler (alternative)
void controlEvent(ControlEvent theEvent) {
  // ... handle boogieRTick ... 
  // Add handlers for notesPerCycle, baseDuration, updatePattern if using this method
}
*/ 