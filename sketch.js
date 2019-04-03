var serial;          // variable to hold an instance of the serialport library
var portName = '/dev/cu.usbmodem1411';  // fill in your serial port name here
var fromSerial = 0;
let radius = [];
let oldRad = 0;
 
function setup() {
  serial = new p5.SerialPort();       // make a new instance of the serialport library
  serial.on('list', printList);  // set a callback function for the serialport list event
  serial.on('connected', serverConnected); // callback for connecting to the server
  serial.on('open', portOpen);        // callback for the port opening
  serial.on('data', serialEvent);     // callback for when new data arrives
  serial.on('error', serialError);    // callback for errors
  serial.on('close', portClose);      // callback for the port closing
 
  serial.list();                      // list the serial ports
  serial.open(portName);              // open a serial port
  createCanvas(800,600);
  background(0);
  for (let i = 0; i< 400; i++){
    radius.push(0);
  }
}

function draw(){
  let smoothRad = 0;
  for (let i = 0; i < radius.length; i++){
    smoothRad += radius[i];
  }
  
  smoothRad = smoothRad / (radius.length + 1);
  console.log(smoothRad); 
  background(0);
	noStroke();
	fill(255,140, 140);
  ellipse(400, 300, smoothRad, smoothRad);
  // oldRad = smoothRad;
}
// get the list of ports:
function printList(portList) {
 // portList is an array of serial port names
 for (var i = 0; i < portList.length; i++) {
 // Display the list the console:
 print(i + " " + portList[i]);
 }
}

function serverConnected() {
  print('connected to server.');
}
 
function portOpen() {
  print('the serial port opened.')
}
 
function serialEvent() {
	var stringFromSerial = serial.readLine();
	if (stringFromSerial.length > 0){
		var trimmedString = trim(stringFromSerial);
    fromSerial = Number(trimmedString);
    fromSerial = Math.abs(fromSerial - 460) * 2;
    // console.log(fromSerial);
    radius.push(fromSerial);
    // console.log(radius);
    radius.splice(0,1);
	}

}
 
function serialError(err) {
  print('Something went wrong with the serial port. ' + err);
}
 
function portClose() {
  print('The serial port closed.');
}
