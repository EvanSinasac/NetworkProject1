Evan Sinasac - 1081418
INFO6016 Network Programming

I used Visual Studios 2019 in Debug x64.

Commands:
Join room: "/join X" (where X is the room name)
Leave room: "/leave X"
Message room: "/message X M" (where M is the message you want to send)

The client is able to keep track of the Socket that it is connected on, manages keyboard input and receiving messages (both non-blocking).  The client won't allow sending messages into a room that the user hasn't joined.  The client sends a message to the server which includes a message_id which tells the server how to handle the message.

When the server receives a message, it looks for the message_id and then parses the rest of the message depending on the protocol that the message_id indicates the message is meant to follow.  When the server receives a message call, it broadcasts the message to all other clients in the same indicated room.  When the server receives a join call, it adds the room_name to the client's list of rooms.  When the server receives a leave call, it removes the room_name from the client's list of rooms.  

GIT HTTPS: 	https://github.com/EvanSinasac/INFO6019-Project1.git
GIT SSH:	git@github.com:EvanSinasac/INFO6019-Project1.git

I used the main code from the select_server example we were given to make it non-blocking.  IDK, somewhere the data isn't being cleaned up and I'm just not fluent in Network Programming to figure out what I'm messing up.  I also don't fully understand how to serialize the data for sending it over the network so I just used a combination of strings and the WSABUF stuff.  I also didn't understand how to make the protocols binary?  I have an issue of not asking for help and just trying to figure things out on my own, this time it lead to this project being a bit of a mess.  I'm sorry for that, I have been staring at this for hours over days making little to no progress and sense out of this.  I think I'm like, one or two steps from everything working nicely, I just can't figure out what those steps are.
