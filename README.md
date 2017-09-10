# TFTP-Server

Authors: Skúli Arnarsson <skulia15@ru.is>
		 Darri Valgarðsson <darriv15@ru.is>
		 Axel Björnsson <axelb15@ru.is>


Answers to questions on the Assignment Description.

	8. (5 points) The RRQ and WRQ have a mode string. Carefully explain and document the meaning
	of the mode. Explain how you handle modes in your implementation and carefully
	justify your decision:
		--
		--
		--
		--
		--

	10. (5 points) Explain how your server handles attempts of uploading files.
		--
		--
		--
		--
		--


Description of Functions:

	void closeFpIfOpen();
		This function closes the file pointer if it is open. This function is usually called when the program encounters an error. This prevents memory leaks.

	void setOpCode(int opcode, char * message);
		This function takes an operation code and a packet as a parameter. The function sets the operation code of the given packet to the operation code given.

	void setBlockNumber(int blockNo, char * message);
		This function sets the given block number for the given packet, It can also set the error code for an error packet since the implementation is the same.
		That is, both the error code in an error packet and a block number in a data packet are stored in the third and fourth byte.

	void sendError(int errCode, char * errorMessage);
		This function constructs an error packet and sends it to the client. It sets the error code by calling setBlockNumber(see explanation above) with the error code errCode sent to the current function as a parameter. It sets the operation code to 4 (error packet) by calling setOpCode with 4 as a a parameter, and the current packet being constructed(the error packet). Lastly the function sends the error packet to the client.

	void sendDataPacket();

	int handleRRQ(char * message, char * directory);

	int handleACK(char * message, int n);

	char * getMode(char * message, char * requestFile);

