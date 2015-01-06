
Installation Instructions for NEditMV for Windows NT and Windows 95.

NEditMV 5.0.1 now support Windows NT and Windows 95. NEditMV is an
enhanced version of NEdit 5.0.1 from Fermi National Accelerator Laboratory.
This is not a port of NEdit to Win32, instead it is a compile of the
X Windows NEdit sources on Win32 using the X windows libraries from 
the Exceed XDK. So this implies that you need an X Server for you PC.
I recommend the Exceed product from Hummingbird. I have been using
if for over a year and I am very happy with it. There is another
commercial X server called Xreflection and there is a free X server
called MI/X.

The installation is the almost identical for Win95 and NT:

You need:

  - The newest version of NEditMV/NT. It can be downloaded from:
	
	ftp://exchange.rational.com/exchange/outgoing/maxv/nedit/5.0.1
	
  - An X window server.

What you have to do:

  - Install NEDIT. Unpack the NEdit ZIP file to c:\nedit for example.
  - Install Exceed or MI/X if it is not already installed.
  - Edit the registry file for the Exceed XDK by following the directions
    in the file.
	    
	c:\nedit\Exceed\user\ExceedXDKRegistrySettings.reg
	
	The Exceed XDK provides an implementation of the X window libraries
	for Win32.
  - Import this file by using the directions in the file. 
  - Set the HOME environment variable to "c:\nedit" or some other
    convenient path. On Win95 insert the command "set home=c:/nedit"
	(slash or backslash, it doesn't matter to NEdit) into autoexec.bat.
	On NT you would add it to the environment via the System Icon in
	the Control Panel.
  - Copy your Unix .nedit and .neditmacro files if you have them
    to directory that you set in the HOME environment variable.
  - With Exceed you can have it load your Unix X defaults. First copy your
    X defaults file from Unix. It is typically called .Xdefaults. Then
	goto the Exceed configuration panel. Select the "Screen Definition"
	icon. Turn on the "Auto Load XRDB" button and enter the path to the
	file that you copied from unix. Unfortunately I tried MI/X and I 
	couldn't find any support for the loading of your X default at startup.

If you have any questions, my email address is max.vohlken@rational.com.

Enjoy.

Max Vohlken
