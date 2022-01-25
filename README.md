README for the HaptiComm Library 

Author: Basil Duvernoy
Date: 2020/04/06

### How to install
1. run the configure.sh file: sudo ./configure.sh
2. copy the script in the root folder: sudo cp libHaptiComm/6_sh/v2/* .

### How to run an example
1. Example for the AD5383 driver :
  - file : libHaptiComm\0_executables\neutral\neutral.cpp 
  - Behavior :
    1. Creation of a AD5383 class object;
	2. Creation of a 2-D matrix with 2048 values (neutral values for the actuator coils);
	3. Sending the matrix to the AD5383 with the AD5383 driver (execute_trajectory).
  
  

2. Example with an alphabet:
  - file : startBlue.sh
  - content : sudo ./build/0_executables/demo_standard/demo 
                --cfg libHaptiComm/4_configuration/configBlue.cfg $@
  - This script launches the bin 'demo' with the configuration file 'configBlue.cfg'.
  - The symbols are defined in the configBlue.cfg file.
  - The speaker interface is defined in the libHaptiComm/0_executables/demo_standard/demo.cpp
  - Behavior : 
    1. Instance of two threads named extract_text and send_to_dac. These threads are used
       to extract keyboard inputs and sending information to the AD5383 driver in parallel;
    2. Creation of the haptiCommConfiguration object;
    2. Instance of the device, waveform, and alphabet classes;
	3. Extract the commandline argument with sepOpt function;
	4. Create the device, waveform, and alphabet objects with the help of the configure method
	   of the HaptiCommConfiguration object;
	5. Put the current program in high priority for ensuring pseudo-real time;
	6. Initialise the terminal so the keyboard input is readable by the program;
	7. Create the threads in relation with the workSymbols and generateSentences functions;
	8. Starts the threads with the use of join functions.
	9. Free allocated memory;
	10. Exit with the status value of the program.
	
    a. generateSentences function:
	  1. Read keyboard input while '*' is not pressed;
	  2. If '*' pressed, exit the thread by closing the current terminal session layer 
	     and warning the other that its job is over;
	  3. Else if the current character is part of the alphabet, it is placed in the shared memory
	     with the function addto_sentence.
		
    b. workSymbols function:
	  1. Create an instance of the AD5383 class;
	  2. Create its object, opens and configures the SPI with the spi_open and configure methods;
	  3. Send to the AD5383 neutral statement to all channels;
	  4. Check for new character until the other thread is over;
	  5. If the other thread send the notification that there is a new character,
	     copy it to its private memory, then delete it from the share memory, 
		 and releases the mutex; 
	  6. If the type of prosody is sentence, wait for a '.' before sending symbols to the 
	     AD5383;
	  7. Else if prosody is type of word, wait for a ' ';
	  8. Else, send the symbol.
	  9. A symbol is sent as follow: 
	    a. Get the array of values with the getl method of the alphabet object;
		b. Execute the symbol with the execute_selective_trajectory method of the AD5383 object;
		c. Add potential overruns (pseudo-realtime issue);
		d. Clear the array of values for the next symbol;
		e. Remove the symbol, that has been sent, of the private memory;
		f. Wait a specific amount of time before sending the next symbol, 
		   depending on the mode of the program (prosody, word, or letter).
	
	
	
	
	
	
	
	
	
	
	
	
