Dhruv Aggarwal
7163546
CS 176A, Winter 2024 - Belding
February 16nd, 2024

My code works as follows:

PingClient.c:

I started out by using the same basic UDP client I wrote for HW2b, but made a few modifications. For the most part, the changes included setting a timeout and formatting the output.
Primarily, I set the socket to have a timeout constraint, that is after 1 second, the socket should timeout on receiving or sending a message.

Within the for loop, we start by sending the ping message, we make sure the timeout is set, then we handle receiving. If nothing is received, we print the appropriate timeout but otherwise I am adding the time value to the array for computation later.
To keep track of minimum and maximum, I simply compared everytime we receive a new value, and accordingly changed the doubles for minimum and maximum. 
Then, we sleep for a second. 

At the very end, I printed the statistics in the desired format and closed the socket.  

