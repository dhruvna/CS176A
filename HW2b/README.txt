Dhruv Aggarwal
7163546
CS 176A, Winter 2024 - Belding
February 2nd, 2024

My code works as follows:

server_c_udp:

The first function, sum_of_digits() simply iterates through a string and searches for ASCII numbers. If it encounters a non-numeric value, it exits as instructed. Otherwise, it sums the numeric values and returns the sum as a string.

In main, we parse arguments and ensure only the filename and port are provided.
Then we setup our UDP socket with the port provided by the command line, and set it to be UDP (SOCK_DGRAM). It binds to any IP address so long as the port is the same as provided.

Our while loop does the hard work. We create some strings to store our values, but then receive from the socket. We sum this value, and send it to the client. If the value exceeds 9, we repeat this process until we have a single digit sum. The server never exits.

client_c_udp:

int main() parses the command line, this time with 3 arguments as we need one for the IP address. We then create our socket exactly the same as the server, BUT we bind to the specific IP provided in the command line. Otherwise, this is identical.
We then take user input and send it to the server. When we receive responses, we print them as the server handles deciding what to provide. If the message printed is either "Sorry..." or a one character long number, we break free of this loop and close our connection/return to exit.

server_c_tcp:

sum_of_digits() is the same, except we are appending a new line to the end of each sum. This is important as TCP packets can arrive all at once and we need a way to parse through them on the client-side later. Otherwise, same as UDP sum function. 
We parse arguments the same as server_c_udp. We create our socket as SOCK_STREAM and configure it with any IP, and the specified port from command line. We bind to the port, and listen for connections. I generically made it 3 but it can be any number.
Then we enter the while loop. Here, we accept any clients (up to 3 still) and parse send the sums to our client. Once we are done, we close the socket connection but keep listening for more potential clients.

client_c_tcp:

This client starts the same, with argc and argv. We create the same socket as server_c_tcp but bind to the specific command line IP as well. Then, we take user input and send it over. This part is the same as UDP.
The hard part came in parsing the packets. We iterate through the buffer, and if we see a new line, replace it with a null terminator so we can keep counting. So long as the string has a message after this, we print is. 
Then we move to the next. This ensures that we can parse through TCP packets even if they arrive all at once. Lastly, close the connection.


UDP was relatively straight forward, but parsing TCP was quite the challenge. 