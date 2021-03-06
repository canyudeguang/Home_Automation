Home Automation
=====================================================================
  This project is based on Qualcomm QCA4020 development kit whose name is Home Automation. The main objective of this demo is to use the QCA4020 development kit to control the Sengled Zigbee bulb and Smart lock using an Android mobile phone and demonstrates smart home. This is done by using an application on the mobile phone (Android O) which will connect the azure cloud. The app give the command to the QCA4020 which will control the sengled bulb via Zigbee and servo motor via PWM.


Instruction:
----------------------

Hardware requirements
--------------
QCA4020 development Kit, Sengled blub, Adaptor, Mobile Phone(Android O), Lamp holder, Trandformer(120V->110V), Servo motor

Software requirements
--------  
1)toolchain
--
The toochain can be downloaded from the ARM website at:  
https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads   
Add the path to toolchain binaries to 'PATH' environment variable.  
	  
2)Python
--
Some of the support scripts are Python-based, Python 2.7.2, or higher. You can download the Python official website.  
After installation, add the path to python to 'PATH' environment variable.  

Assembly Instructions
-----------------------------------------------------------------------------
Install the jumper of QCA4020 development kit and connect the servo motor to QCA4020 as described in the section  
"Build/Assembly Instructions" of "CDB20_Project_Content_HomeAutomation.docx".

Usage Instructions
--------------------------
(1)Download the code from github according to this repositary.  

(2)Compile the code and flash the image as described in "Hello world with QCA4020 Development" project. 

(3)Power on the QCA4020 via the power button,and the QCA4020 will build Zigbee network automatically.  

(4)Power on the Zigbee bulb,and it will join the Zigbee network automatically.  

(5)Using “Azure_console” app on the mobile phone to control on or off status of bulb via clicking “Open” or  “Close” button.  

(6)Using “Azure_console” app on the mobile phone to control color temperature via setting the values ranged from “0x0000~0xFFFF” .  

(7)Using “Azure_console” app on the mobile phone to control dim via setting the values ranged from “0~255”.  

(7)Using “Azure_console” app on the mobile phone to control servo motor via clicking “Open” or  “Close” button.  
