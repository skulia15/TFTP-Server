# TFTP-Server

Authors: Skúli Arnarsson <skulia15@ru.is>
		 Darri Valgarðsson <darriv15@ru.is>
		 Axel Björnsson <axelb15@ru.is>


Answers to questions on the Assignment Description.

	8. (5 points) The RRQ and WRQ have a mode string. Carefully explain and document the meaning
	of the mode. Explain how you handle modes in your implementation and carefully
	justify your decision:
		The mode represents the format on which the file is, or should be received on. We open netascii files with the command fopen(path, "r") because different operating systems have different ways of storing text and this does the correct translation so we dont need to think about that. We open octet with the command fp = fopen(path, "rb"), this opens the file exactly as is and doesn't do any translations because we need the file to be the exact copy of the original. 

	10. (5 points) Explain how your server handles attempts of uploading files.

		File uploading is not allowed on our server, so if it receives a write request or a data packet, it sends back an error packet with error code 4, illegal operation, and an error message saying that data upload is illegal.


Description of Functions:

	void closeFpIfOpen();
		This function closes the file pointer if it is open. This function is usually called when the program encounters an error. This prevents memory leaks.

	void setOpCode(int opcode, char * message);
		This function takes an operation code and a packet as a parameter. The function sets the operation code of the given packet to the operation code given.

	void setBlockNumber(int blockNo, char * message);
		This function sets the given block number for the given packet, It can also set the error code for an error packet since the implementation is the same.
		That is, both the error code in an error packet and a block number in a data packet are stored in the third and fourth byte.

	void sendError(int errCode, char * errorMessage);
		This function constructs an error packet and sends it to the client. It sets the error code by calling setBlockNumber() (see explanation above) with the error code errCode sent to the current function as a parameter. It sets the operation code to 4 (error packet) by calling setOpCode() with 4 as a a parameter, and the current packet being constructed(the error packet). Lastly the function sends the error packet to the client.

	void sendDataPacket();
		Constructs a data packet by setting the opcode and the block number, reading data from the file to the buffer and then reading data from the buffer into the data packet and lastly sending the data packet to the client.

	int handleRRQ(char * message, char * directory);
		This function handles the read requests. When an operation code with value 1 received. It gets the name of the file requested from the message. Then constructs a full path by concatenating available directory and the requested file with the appropriate error checking. Then the mode is fetched by calling getMode. The file is then opened and a data packet is then sent tp the client by calling sendDataPacket().

	int validFilename(char *requestFile);
		Checks whether the string contains a "." character. That is not allowed in a filename, since it would allow you to access directories outside of the allowed ones.

	int handleACK(char * message, int n);
		Checks if the last packet has been received.
		Then checks if the block number of the acknowledgement packet
		is the same as in the block number of the most recently sent data packet
		if it is the same the next data packet is sent, if it is one less then we
		resend the previous data packet, else an error is thrown.

	char * getMode(char * message, char * requestFile);
		Fetches the mode from the given message. Mode is found in the header, after the 2 opcode bytes, the filename and the null byte.

	int main(int argc, char *argv[]);
		Checks for the number of provided arguments. It retrieves the provided directory. It creates and binds a UDP socket and configures by settings.
		Then an infinite loop starts, that exists only on an error condition.
		The loop: It waits for a message from the client. When it received one, it retrieves the operation code from the message and handles the received message differently based on the operation code. If the code corresponds to 1(RRQ), we call to handleRRQ(). If the operation code corresponds to 4 (ACK) then we call to handleACK(). If the operation code corresponds to anything else we send an error. When the loop exits we close the socket.

