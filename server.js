/*
	doors -- TCMaker door system backend
	
	based on 'Socket.io example' by Tom Igoe
	https://github.com/tigoe/NodeExamples

	with additional duct tape, bailing wire,
	poor coding practices and bugs by Scott Hill and Jude Dornisch

	to start:
	node server.js /dev/tty.something
*/

var express = require('express');		// include express.js
var app = express();					// make an instance of express.js
var http = require('http').Server(app);	// include http and make a server instance with the express instance
var io = require('socket.io')(http);	// include socket.io and make a socket server instance with the http instance

var serialport = require("serialport");
var SerialPort  = serialport.SerialPort;
var sqlite3 = require('sqlite3');    // remove '.verbose()' to deactivate debug messages 
var db = new sqlite3.Database("/opt/doors/doors.sqlite"); // opens the database

global.lastStatusTime = "";  // time string from last status update
global.doorState1 = 1;       // state of each door:
global.doorState2 = 1;       // 1 == locked; 0 == open

app.use("/", express.static(__dirname));

// send the index page if you get a request for / :
//app.get('/', sendIndex);
app.get('/', sendIndex);

// the third word of the command line command is serial port name:
var portName = process.argv[2];				  
// print out the port you're listening on:
writeAuditData("opening serial port: " + portName);

// open the serial port. Uses the command line parameter:
var myPort = new SerialPort(portName, { 
	baudRate: 9600,
	// look for return and newline at the end of each data packet:
	parser: serialport.parsers.readline("\r\n") 
}, false);

//
// callback functions for 'get /' requests:
//
function sendIndex(request, response){
	response.sendFile(__dirname + '/index.html');
}

//
// socket setup and message handlers
//
io.on('connection', function(socket){
	writeAuditData('socket connected from ' + socket.handshake.address);

	// send something to the web client with the data:
	socket.emit('message', "Hello, " + socket.handshake.address);
		
	socket.on('setDoorLock', function(cmd) {
		writeAuditData('socket message: setDoorLock ' + cmd);
		sendToSerial(cmd);
	});

	socket.on('lockAllDoors', function(){
		writeAuditData('socket message: lockAllDoors');
		sendToSerial("D1L\r\nD2L");
	});

	socket.on('addTag', function(tag, priority) {
		writeAuditData('socket message: addTag ' + tag + ' ' + priority)
		db.run("INSERT OR REPLACE INTO tags VALUES (?, ?);", tag, priority, function(err){
			if (err == null){
				writeAuditData("sqlite: OK");
			} else {
				writeAuditData("ERR sqlite: " + err);
			}
		});
	});

	socket.on('getStatus', function() {
		writeAuditData('socket message: getStatus');
		sendToSerial("ST");
	});
});

//
// Serial port connect and event handlers
//
myPort.open(function (error) {
	if (error) {
		writeAuditData("ERR myPort: " + error);
	} else {
		// Event fires when CRLF-terminated data appears at the serial port
		myPort.on('data', function (data) {
			// for debugging, you should see this in Terminal:
			writeAuditData("myPort: received " + data);

			// Check the string for its message type:
			var messageType = data.slice(0,2);

			if (messageType == "ST") {
			  	parseStatus(data);
			  	io.sockets.emit('doorState1', global.doorState1);
			  	io.sockets.emit('doorState2', global.doorState2);
			  	io.sockets.emit('lastStatusTime', global.lastStatusTime);
			  	//console.log(global.doorState1 + ' ' + global.doorState2 + ' ' + global.lastStatusTime);
			}

			if (messageType.match(/^R/gi)) {
			  	// Strip apart the string retreived from the serial port
			  	var keypad = data.slice(0,2);  // R1 or R2
			  	var door = data.slice(1,2);    // 1 or 2, corresponding to the keypad
			  	var command = data.slice(2,3); // C == keypress; T == tag read
			  	var cmdvalue = data.slice(3);  // keypad input or derived tag number
			
			  	// Tag read handler
			  	// Query the DB for the tag. If no result, send 'DxT0'
			  	// If found, send 'DxTy', where x == door number, y == priority value from DB
			  	if (command == "T") {
			  		db.get("SELECT priority FROM tags WHERE tagnumber = ?;", cmdvalue, function (err, row){
			  			if (typeof row == "undefined") {
			  				writeAuditData("ERR NO_TAG: " + cmdvalue);
			  				sendToSerial("D" + door + "P0");
			  			} else {
			  				writeAuditData("Found tag: " + cmdvalue);
			  				sendToSerial("D" + door + "P" + row.priority);
							db.run("INSERT INTO activity (tagnumber) VALUES (?);", cmdvalue, function(err){
								if (err == null){
									return;
								} else {
									writeAuditData("ERR sqlite: " + err);
								}
							});
			  			}
			  		});
			  	}
			}
		  // send a serial event to the web client with the data:
		  //socket.send('message',data);	
		});
	}
});

// If '.verbose()' is set on the sqlite3 library load, this event will fire when a query is sent
db.on('trace', function (sqlstring) {
	console.log(sqlstring);
});

function sendToSerial(data) {
  writeAuditData("sendToSerial: " + data);
  myPort.write(data + "\r\n");
}

function parseStatus(data) {
	//console.log('parseStatus: ' + data);
	global.doorState1 = data.slice(2,3);
	global.doorState2 = data.slice(3,4);
	global.lastStatusTime = data.slice(5);
}

function writeAuditData(text) {
	console.log(text);
	io.sockets.emit('conmsg', text + '\r\n');
	db.run("INSERT INTO audit (text) VALUES (?);", text, function(err){
		if (err == null){
			return;
		} else {
			console.log("ERR writeAuditData: " + err);
		}
	});	
}

// listen for incoming server messages:
http.listen(8080, function(){
  writeAuditData('listening on port 8080');
});
