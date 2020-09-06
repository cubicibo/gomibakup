## gomibakup
Gomibakup is a work in progress dumping device/system for the NES & Famicom. It runs on ChibiOS and Micropython combined and require no host application to operate. The user just connects with a terminal session to Micropython, paste the Python script and save the data coming back from a second terminal session to a NES rom.

#### Folder setup
<pre>
./gomibakup/ +
	    |- lib/ +
		    |- ChibiOS/
	   	    |- micropython/
    	    |- Micropython_ChibiOS
</pre>

where ChibiOS is a clone of the git repository (contains demos/, doc/, os/...)