# CamTest
## Overview
CamTest is a command line application for exercising the Sentera Protocol ICD for camera control.
The application supports both Windows and Linux operating systems and the following cameras:
* Torpedo base single, double, and quadruple
* QX-10 HW1
* QX-10 HW2
* SnapCam

## Compiling and Executing the Application
### Connect
Connect the camera directly to a UNIX or Windows PC via ethernet or ensure both devices are active and 
operating on the same network.  In order to send and recieve imaging/status packets from the sensor,
the PC must be assigned a specific IP address based on the camera's platform.

Platform	|	Camera IPv4			|	PC IPv4
:---:		|	---					|	---
Phoenix 2	|	192.168.143.141		|	192.168.143.10
Omni		|	192.168.143.141		|	192.168.143.10
Indago		|	192.168.168.141		|	192.168.168.10
Double4k	|	192.168.168.141		|	192.168.168.130

On Windows systems, a static IP address is assigned to the Wi-Fi network interface via the Network 
Connections pane of the Control Panel.  The Network Connections pane can be accessed by searching "View 
network connections" in the Start menu, opening "ncpa.cpl" via a Run instance, or opening Control Panel 
and navigating Network and Internet > Network and Sharing Center > Change adapter Settings.  From the 
Network Connections pane, right-click the Wi-Fi interface and click Properties.  Double-click 
the entry labelled Internet Protocol Version 4 (TCP/IPv4). In the new properties window, select the 
radio button labelled "Use the following IP address." For *IP address* enter the address corresponding
to the relevant system shown above and for *Subnet mask* enter "255.255.255.0" as shown the the following 
screen capture.  Click Ok to save the network settings.

On UNIX systems, setting a static IP address varies across distributions. Please refer to the distrubution
documentation or webpage for assistance.

### Compile
#### Windows
On Windows systems, compiling a C++ program requires additional software.  Refer to the following
link for instructions on using the command-line compiler packaged with Microsoft's free IDE, Visual
Studio Community Edition: https://msdn.microsoft.com/en-us/library/ms235639.aspx.

In short:

1. Install Visual Studio Community  
2. Open the Windows search bar and type "command".  Select "VS2015 x86 Native Tools Command Prompt".  
   Doing so opens a command prompt with environment variables set to enable command line compiling.  
3. `> cd` to the camtest source directory.  
4. `> nmake -f makefile.windows`  

You can also provide the following arguments to `nmake`: `all`, `clean`, and `camtest`.

#### Unix
On UNIX systems, simply open the terminal and navigate to the directory containing the CamTest project
files and issue the following command.

'gpp -o main CameraTest.cpp crc.c''

Upon success, the command produces the executable *main*.
	
### Execute
Following compilation, the application is intialized using the following template.

`[CamTest Executable] [Camera IPv4] [Port] [PC IPv4] [# of Cameras]`

where [CamTest Executable] is the filepath of the compiled executable, [Camera IPv4] is the IP address 
of the camera (see *Connecting* above), [Port] is the specific port the camera utilizes to send packets 
(default is 60530), [PC IPv4] is the static IP address of the PC (see *Connecting* above), and 
[# of Cameras] is the discrete number of connected cameras/sensors.

For example, the following command might initalize CamTest for a dual-sensor camera from an Omni systm
on a Windows PC:

`CamTest.exe 192.168.143.141 60530 192.168.143.10 2`

## Operating the Application
### Status
Upon initalizing the application, CamTest provides a realtime display of the imager statuses in the following format:

`Sentera # [S-001   0% /  0 GB]`

where the number following *Sentera* represents the imager ID, the S (or R) represents session stopped (or running), The number next to the S (or R)
represents the current image count, the percentage represents the portion of card memory consumed, and the number preceding GB represents card memory size.

In the case of a communication error, the following status will be displayed:

`Sentera # [No Communication]`

To resolve the issue, check the connection and restart the application.

### Commands
CamTest provides a wide range of commands for testing sensors.  These commands and their arguments are described below.

Command							| Description                        
--------------------------------|------------------------------------		
ea (exposure adjust)			| Adjusts exposure (given time, analog gain, and digital gain)
el								| Sends elevation metadata
files							| Prints recent image filepaths for all imagers
focus							| Opens a new focus session in video mode
is (imager select)				| Selects active sensors by adjusting an 8-bit mask where the least significant bit is Camera 1; accessed by other commands
loc (set location)				| Spoof autopilot info packet
preview							| Configures imager video preview stream.
quit							| Quits CamTest application
session							| Begins new imaging session
sf (still focus)				| Begins a new focus session in still image mode
spoof							| Spoofs an autopilot sending periodic triggers
status							| Prints all status information for the active sensors
trigger							| Begins/stops snapshot triggering			
video							| Begins new video session
videotime						| Begins new video session with a timestamp
videoadjust						| Adjusts bitrate, group of pictures, electronic image stabilization, and brightness
     bitrate					| Adjusts bitrate of video
     gop						| Adjusts group of pictures (decrease for increased quality of dynamic scenes)
     estab						| Toggles electronic image stabilization
     brightness					| Adjusts auto exposure target [1:4095] and gain [1:255]
videoadvanced					| Adjusts all *videoadjust* settings as well as horizontal and vertical resolution
zoom							| Increases/decreases the field of view at a constant rate or incrementally

#### ea

Adjusts imager exposure according to user supplied exposure time, analog gain, and digital gain.

Argument				| Values
------------------------|---------------------------------------
Exposure                | 0 = auto; 100 to 320,000 = manual
Analog gain				| 0 to 3 (0=1x, 1=2x, 2=4x, 3=8x)
Digital gain			| 0 to 255 (32 = 1x)

#### el

Sends elevation metadata to the payload.

Argument				| Values
------------------------|---------------------------------------
Mode	                | Camera AGL (0) - use provided cameraAGL, Terrain MSL (1) - subtract terrainMSL from an altitude provided elsewhere
, Camera MSL (2) - subtract terrainMSL from cameraMSL provide here
Valid bit flag			| Camera MSL (4), Terrain MSL (2), Camera AGL (1)
Camera AGL (m)			| Camera height above ground level
Terrain MSL (m)			| Terrain height aboce MSL
Camera MSL (m)			| Camera height above MSL

#### files

Prints recent image filepaths for all imagers to the console.

#### focus

Opens a new focus session in video mode

Argument				| Values
------------------------|---------------------------------------
Open					| 0
Exposure time (us)		| 0 = auto; 100 to 320,000 = manual

#### is

Imager select; selects active sensors by adjusting an 8-bit mask where the least significant bit is Camera 1; accessed by other commands.
ex. 0x02 -> selects for camera 2, so any following commands will only apply to camera 2.

Argument				| Values
------------------------|---------------------------------------
New mask value			| 0x00 - 0xFF

#### loc

Spoofs autopilot info packet.

Argument				| Values
------------------------|---------------------------------------
Interval (micro-seconds)| A positive integer

#### preview

Configures imager video preview stream.

Argument				| Values
------------------------|---------------------------------------
Enable					| 0 = disable; 1 = enable
IP Address				| Streaming video destination IPv4 address (0 = default)
Port					| Streaming video destination port (0 = default)
Cam Pos (4k only)		| 0 = *Use Previous* or *Default*; 1 = Disabled; 2 = Fullscreen; 3 = Lower Right; 4 = Top; 5 = Bottom; 6 = Overlay 
Cam Opt (4k only)		| 0 = *Use Previous* or *Default*; 1 = Normal (NIR/RGB); 2 = Live Colormapped NDVI  Fixed Color Map (NIR Only)
Overlay	(4k only)		| 0 = No change; 1 = disable; 2 = enable 

#### quit

Exits the CamTest application.

#### Session

Begins a new imaging session.

Argument				| Values
------------------------|---------------------------------------
Control					| 0 = open; 1 = close; 2 = kill
Session Name			| User specified
Append					| Appends UTC time data to session packet: 0 = no; 1 = yes

#### sf 

Still focus - begins a new focus session in still image mode.

Argument				| Values
------------------------|---------------------------------------
Exposure Time (useconds)| A positive integer

#### spoof							

Spoofs an autopilot sending periodic triggers.

Argument				| Values
------------------------|---------------------------------------
Interval (useconds)		| A positive integer indicating time between spoofed packets

#### status		
					
Prints all status information for the active sensors to the console.

#### trigger 

Begins/stops snapshot triggering.

Argument				| Values
------------------------|---------------------------------------
Toggle Trigger			| 0 = disable; 1 = single; 2 = continuous
Interval (ms)			| A positive integer indicating time between snapshots

##### video	

Begins new video session.

Argument				| Values
------------------------|---------------------------------------
Open video session		| 0 = open
Exposure time (useconds)| A positve integer indicating exposure time


#### videotime

Begins new video session with a timestamp.

Argument				| Values
------------------------|---------------------------------------
Open video session		| 0 = open
Exposure time (useconds)| A positve integer indicating exposure time

#### videoadjust

Adjusts bitrate, group of pictures, electronic image stabilization, and brightness.
     
Argument				| Values
------------------------|---------------------------------------
Bitrate					| See *bitrate*
Group of Pictures		| See *gop*
Estab					| See *estab*
Auto Exposure			| See *bitrate*
Bitrate					| See *bitrate*

##### videoadvanced	

Adjusts all *videoadjust* settings as well as horizontal and vertical resolution.

Argument				| Values
------------------------|---------------------------------------
Exposure time (useconds)| A positve integer indicating exposure time
Bitrate					| See *bitrate*
Group of Pictures		| See *gop*
Metadata Source			| 0 = none; 1 = Sentera ICD; 2 = Dyn Test; 3 = Stat Test
Estab					| See *estab*
Auto Exposure			| See *bitrate*
Gain					| 1 to 255 (rec = 64)
Horizontal Res. (px)	| 128 to 1248
Vertical Res. (px)		| 128 to 720

#### bitrate

Adjusts bitrate of video.

Argument				| Values
------------------------|---------------------------------------
Bitrate (bps)			| 100,000 to 10,000,000

##### gop						

Adjusts group of pictures.

Argument				| Values
------------------------|---------------------------------------
Group of pictures		| 1 to 30 (decrease for increased quality of dynamic scenes)

##### estab

Toggles electronic image stabilization.

Argument				| Values
------------------------|---------------------------------------
Toggle					| 0 = disable; 1 = enable

##### brightness

Adjusts auto exposure target [1:4095] and gain [1:255].

Argument				| Values
------------------------|---------------------------------------
Auto Exposure target	| 1 to 4095 (rec = 1024)

##### zoom

Increases/decreases the field of view at a constant rate or incrementally in discrete steps.

Argument				| Values
------------------------|---------------------------------------
Mode					| 1 = zoom at a rate; 2 = zoom in steps
Rate					| -X = Zoom wider FOV; +X = Zoom narrower FOV; 0  = Stop
Steps					| -X = Zoom wider FOV; +X = Zoom narrower FOV
