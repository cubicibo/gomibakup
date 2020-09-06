## gomibakup
#### Folder setup
<pre>
./gomibakup/ +
	    |- lib/ +
		    |- ChibiOS/
	   	    |- micropython/
    	    |- Micropython_ChibiOS
</pre>

where ChibiOS is a clone of the git repository (contains demos/, doc/, os/...)

#### Eclipse setup
Import the project as a "makefile project with existing code", set the proper compiler. When the project is imported, add a linked folder that points to the "lib" folder, so that Eclipse can see the library.

Depending of your setup, the project might require to be built directly from the terminal rather than Eclipse.
