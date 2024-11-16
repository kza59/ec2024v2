const express = require("express"); //import the express module
const http = require("http"); //import the http module
const { Server } = require("socket.io"); //import the Server class from the socket.io module
const users = { // object to store registered users and their passwords
  "headquarters": "1",
  "spacecraft": "2",
  "apollo" : "apolloPassword"
};
const app = express(); //create an express app instance
const server = http.createServer(app); //create an HTTP server using the express app
const io = new Server(server); //create a socket.io server instance attached to the HTTP server

app.use(express.json()); //middleware to parse JSON request bodies
// app.use(express.static(path.join(__dirname, 'public'))); //serve static files

const onlineUsers = {}; //object to keep track of online users by their email

app.get("/", (req, res) => { //handle GET requests to the root path
  res.sendFile(__dirname + "/index.html"); //send the index.html file as the response
});

//user login endpoint, extract email from the request body, check if the user is already online, add the user to onlineUsers
//used to send data to the server
app.post("/login", (req, res) => {
  const { email, password } = req.body; //extract email and password from the request body
  if (!email || !password) {
    return res.status(400).json({ error: "Email and password are required" }); // Return an error if missing
  }

  if (!users[email] || users[email] !== password) { //check if the email exists and password matches
    return res.status(401).json({ error: "Invalid userID or password" }); //return an unauthorized error
  }

  if (onlineUsers[email]) { //check if the user is already logged in
    return res.status(403).json({ error: "User is already logged in elsewhere" }); // Deny login if the user is already online
  }

  //add the user to the onlineUsers object
  onlineUsers[email] = { email, password, socketId: null };
  console.log(`User logged in: ${email}`);
  res.status(200).json({ message: "Logged in successfully", email });
});

//get online users
app.get("/users", (req, res) => { //handle GET requests to the /users endpoint
  res.json(Object.values(onlineUsers).map((user) => user.email)); //return a list of online users' emails
});

//serve the chat page
const path = require("path"); //import the path module to handle file paths

app.get("/chat", (req, res) => { //handle GET requests to the /chat endpoint
  res.sendFile(path.join(__dirname, "chat.html")); //send the chat.html file as the response
});

io.on("connection", (socket) => { //listen for new connections to the socket.io server
  console.log("A user connected:", socket.id); //log the socket ID of the connected user

  socket.on("login", (email) => { //listen for the "login" event from the client
    if (onlineUsers[email]) { //check if the user is already in the onlineUsers object
      onlineUsers[email].socketId = socket.id; //set the socket ID for the logged-in user
      io.emit("updateUsers", Object.values(onlineUsers).map((user) => user.email)); //emit the updated list of online users to all clients
      console.log(`${email} is now online`); //log the user's login
    }
  });

  socket.on("sendMessage", ({ to, from, message }) => { //listen for the "sendMessage" event from the client
    const recipient = Object.values(onlineUsers).find((user) => user.email === to); //find the recipient by email
    if (recipient && recipient.socketId) { //check if the recipient is online
      io.to(recipient.socketId).emit("receiveMessage", { from, message }); //send the message to the recipient
    }
  });

  socket.on("disconnect", () => { //listen for the "disconnect" event when the user disconnects
    const email = Object.keys(onlineUsers).find( //find the email of the disconnected user
      (key) => onlineUsers[key].socketId === socket.id //check if the socket ID matches
    );
    if (email) { //if the user is found
      delete onlineUsers[email]; //remove the user from the onlineUsers object
      io.emit("updateUsers", Object.values(onlineUsers).map((user) => user.email)); //emit the updated list of online users
      console.log(`${email} disconnected`); //log the user's disconnection
    }
  });
});

server.listen(3000, () => { //start the server and listen on port 3000
  console.log("Server running on http://localhost:3000"); //log the server's URL to the console
});
